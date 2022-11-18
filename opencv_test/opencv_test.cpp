#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/objdetect.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <math.h>
#include <chrono>
#include <stdlib.h>
#include <vector>
#include <thread>

//Constants

enum Screen { width = 1918, height = 775 };
enum Eye_prop 
{
	eye_sep = 20,
	eye_radius = 9,
	eye_socket_radius = 13,
	max_def_x = 70,
	max_def_y = 70,
	eye_def_max = 20,
	wait_time_milis = 34,
	step = 35
};
enum Eye_blink 
{
	blink_tgt = 500,
	blink_delta = 17,
	blink_offset = 100
};

typedef std::chrono::milliseconds milis;

using namespace cv;
using namespace std;

//Globals
sf::Vector2f avg = { 0, 0 };
sf::Vector2f head_past = { 0, 0 }; //Past eye position. Needed to decrease jitter
sf::Vector2f tgt = { 0, 0 };
sf::Vector2f matsz = { 640, 480 };
bool draw = true;
float frame_time = 0;

void drawEyes()
{
	//Setting up the window
	sf::RenderWindow window;
	window.setFramerateLimit(35);
	window.setVerticalSyncEnabled(true);
	window.create(sf::VideoMode(width, height), "Output");
	//Time points for blinking
	auto t1 = chrono::high_resolution_clock::now();
	auto blink_time = chrono::high_resolution_clock::now();
	sf::Vector2f eye_def = { 0, 0 };
	bool blink = false;
	bool backwards = false;
	int blink_duration = 2000;
	float rect_current = 0;
	while (draw && window.isOpen())
	{
		sf::Vector2u w_size = window.getSize();
		int c_width = w_size.x;
		int c_height = w_size.y;
		float delta = sqrtf(pow(avg.x - head_past.x, 2) + pow(avg.y - head_past.y, 2));
		//Only track movement above a threshold to reduce jitter
		if (delta > eye_def_max)
		{
			t1 = chrono::high_resolution_clock::now();
			head_past = { avg.x, avg.y };
		}
		else
		{
			auto t_tmp = chrono::high_resolution_clock::now();
			std::chrono::duration<float> fsec = t_tmp - t1;
			milis ms = std::chrono::duration_cast<milis>(fsec);
			//If enough time has passed since movement was tracked, deflect eyes
			if (ms.count() >= wait_time_milis)
			{
				//Convert camera coordinates to window coordinates
				float scale_fx = c_width / matsz.x;
				float scale_fy = c_height / matsz.y;
				int avg_conv_x = avg.x * scale_fx;
				int avg_conv_y = avg.y * scale_fy;
				//Convert head position to eye deflection
				tgt.x = ((c_width / 2) - avg_conv_x) * (2 * float(max_def_x) / float(c_width));
				tgt.y = (avg_conv_y - (c_height / 2)) * (2 * float(max_def_y) / float(c_height));
			}
		}
		auto blink_tmp = chrono::high_resolution_clock::now();
		std::chrono::duration<float> fsec = blink_tmp - blink_time;
		milis ms = std::chrono::duration_cast<milis>(fsec);
		if (ms.count() > blink_duration)
		{
			blink = true;
			blink_time = blink_tmp;
			blink_duration = ((rand() % 50) + 20) * 100;
		}
		//Draw everything
		eye_def.x = eye_def.x + step * 0.0001 * (tgt.x - eye_def.x);
		eye_def.y = eye_def.y + step * 0.0001 * (tgt.y - eye_def.y);
		int tmp_r = eye_socket_radius * 0.01 * c_width;
		int tmp_r1 = eye_radius * 0.01 * c_width;
		int socket_x = eye_sep * 0.01 * c_width;
		sf::CircleShape socket1(tmp_r);
		sf::CircleShape socket2(tmp_r);
		sf::CircleShape eye1(tmp_r1);
		sf::CircleShape eye2(tmp_r1);
		socket1.setFillColor(sf::Color(255, 255, 255));
		socket2.setFillColor(sf::Color(255, 255, 255));
		eye1.setFillColor(sf::Color(0, 0, 0));
		eye2.setFillColor(sf::Color(0, 0, 0));
		socket1.setPosition((c_width / 2) - socket_x - tmp_r, c_height / 2 - tmp_r);
		socket2.setPosition((c_width / 2) + socket_x - tmp_r, c_height / 2 - tmp_r);
		eye1.setPosition((c_width / 2) - socket_x - tmp_r1 + eye_def.x, c_height / 2 - tmp_r1 + eye_def.y);
		eye2.setPosition((c_width / 2) + socket_x - tmp_r1 + eye_def.x, c_height / 2 - tmp_r1 + eye_def.y);
		socket1.setPointCount(int(100 * c_width / 800));
		socket2.setPointCount(int(100 * c_width / 800));
		eye1.setPointCount(int(50 * c_width / 800));
		eye2.setPointCount(int(50 * c_width / 800));
		window.draw(socket1);
		window.draw(socket2);
		window.draw(eye1);
		window.draw(eye2);
		if (blink)
		{
			float rect_step = 0;
			sf::RectangleShape tmp_rect(sf::Vector2f(socket_x + tmp_r * 4 + eye_sep, rect_current));
			tmp_rect.setPosition((c_width / 2) - socket_x - tmp_r, c_height / 2 - tmp_r);
			tmp_rect.setFillColor(sf::Color(0, 0, 0));
			window.draw(tmp_rect);
			if (rect_current >= tmp_r * 2 + blink_offset)
			{
				backwards = true;
			}
			if (backwards)
			{
				rect_step = -c_height * 0.00001 * blink_delta;
			}
			else
			{
				rect_step = c_height * 0.00001 * blink_delta;
			}
			if (rect_current <= 0 && backwards)
			{
				blink = false;
				backwards = false;
				rect_current = 0;
			}
			else
			{
				rect_current += rect_step;
			}
		}
		sf::Event event;
		while (window.pollEvent(event))
		{
			//"close requested" event: we close the window
			if (event.type == sf::Event::Closed)
			{
				draw = false;
				window.close();
			}
			if (event.type == sf::Event::Resized)
			{
				//Update the view to the new size of the window
				sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
				window.setView(sf::View(visibleArea));
			}
		}
		window.display();
		window.clear();
	}
}

int main()
{
	CascadeClassifier cascade;
	//Loading data for ai detection
	cascade.load("C:/opencv/sources/data/haarcascades/haarcascade_frontalface_alt.xml");
	vector<Rect> faces;
	VideoCapture cap(0);
	if (cap.isOpened())
	{
		thread eye_thread(drawEyes);
		while (1)
		{
			Mat frame;
			sf::Vector2f tmp = { 0, 0 };
			bool check = cap.read(frame);
			GaussianBlur(frame, frame, Size(5, 5), 0);
			if (!check)
			{
				cout << "Capture failed";
				break;
			}
			cascade.detectMultiScale(frame, faces, 1.7, 1.9, CASCADE_SCALE_IMAGE, Size(50, 50));
			for (int i = 0; i < faces.size(); i++) //Calculate average position of faces
			{
				int x = faces[i].x + faces[i].width / 2;
				int y = faces[i].y + faces[i].height / 2;
				tmp.x += x;
				tmp.y += y;
			}
			if (faces.size())
			{
				tmp.x /= faces.size();
				tmp.y /= faces.size();
				circle(frame, Point(tmp.x, tmp.y), eye_radius, Scalar(0, 0, 0), 7);
				avg = tmp;
			}
			//Display everything
			imshow("Actual", frame);
			matsz.x = frame.size().width;
			matsz.y = frame.size().height;
			if (waitKey(1) == 27)
			{
				draw = false;
				eye_thread.join();
				break;
			}
		}
	}
	else
		return -1;
	return 0;
}