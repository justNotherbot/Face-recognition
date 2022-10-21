#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <math.h>
#include <chrono>
#include <vector>

//Constants

enum Screen { width = 640, height = 480 };
enum Eye_prop {
	eye_sep = 120,
	eye_radius = 90,
	max_def_x = 70,
	max_def_y = 70,
	eye_def_max = 20,
	wait_time_milis = 60,
	step = 8 };

typedef std::chrono::milliseconds milis;

using namespace cv;
using namespace std;

struct vec2d
{
	float x, y;
};

int main()
{
	CascadeClassifier cascade, nestedCascade;
	//Loading data for ai detection
	cascade.load("C:/opencv/sources/data/haarcascades/haarcascade_frontalface_alt.xml");
	vector<Rect> faces;
	VideoCapture cap(0);
	auto t1 = chrono::high_resolution_clock::now();
	if (cap.isOpened())
	{
		vec2d eye_def = { 0, 0 };
		vec2d tgt = { 0, 0 };
		vec2d head_past = { 0, 0 }; //Past eye position. Needed to decrease jitter
		while (1)
		{
			Mat background(height, width, CV_8UC3, Scalar(255, 255, 255));
			Mat frame;
			vec2d avg = {0, 0};
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
				avg.x += x;
				avg.y += y;
			}
			if (faces.size())
			{
				avg.x /= faces.size();
				avg.y /= faces.size();
				float delta = sqrtf(pow(avg.x - head_past.x, 2) + pow(avg.y - head_past.y, 2));
				circle(frame, Point(avg.x, avg.y), eye_radius, Scalar(0, 0, 0), 7);
				if (delta > eye_def_max)
				{
					t1 = chrono::high_resolution_clock::now();
					head_past = { avg.x, avg.y };
				}
				else
				{
					//Wait for a certain time period if large movement was encountered
					auto t_tmp = chrono::high_resolution_clock::now();
					std::chrono::duration<float> fsec = t_tmp - t1;
					milis ms = std::chrono::duration_cast<milis>(fsec);
					if (ms.count() >= wait_time_milis)
					{
						//Convert head position to eye deflection
						tgt.x = (avg.x - (width / 2)) * (2 * float(max_def_x) / float(width));
						tgt.y = (avg.y - (height / 2)) * (2 * float(max_def_y) / float(height));
					}
					cout << avg.x << ' ' << avg.y << "\n";
				}
			}
			//Display everything
			eye_def.x = eye_def.x + step * 0.01 * (tgt.x - eye_def.x);
			eye_def.y = eye_def.y + step * 0.01 * (tgt.y - eye_def.y);
			circle(background, Point((width / 2) - eye_sep + int(eye_def.x), height / 2 + int(eye_def.y)), eye_radius, Scalar(0, 0, 0), -1);
			circle(background, Point((width / 2) + eye_sep + int(eye_def.x), height / 2 + int(eye_def.y)), eye_radius, Scalar(0, 0, 0), -1);
			imshow("Actual", frame);
			imshow("Display Window", background);
			if (waitKey(1) == 27)
				break;
		}
	}
	else
		return -1;
	return 0;
}