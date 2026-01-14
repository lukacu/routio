#include <unistd.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <ctime>
#include <chrono>
#include <opencv2/opencv.hpp>

#include <routio/client.h>
#include <routio/datatypes.h>
#include <routio/helpers.h>
#include <routio/camera.h>

using namespace std;
using namespace routio;
using namespace cv;

int main(int argc, char** argv) {

    SharedClient client = routio::connect(string(), "videoclient");

    shared_ptr<Frame> frame;
    Mat canvas;
    bool incoming = false;

    bool headless = (NULL == getenv("DISPLAY"));

    function<void(shared_ptr<Frame>)> frame_callback = [&](shared_ptr<Frame> m) {

        frame = m;
        incoming = true;

    };

    SharedTypedSubscriber<Frame> frame_subscriber = make_shared<TypedSubscriber<Frame> >(client, "camera", frame_callback);

    while (true) {

        if (incoming) {

            std::time_t tt = std::chrono::system_clock::to_time_t ( frame->header.timestamp );

            if (!headless) {
                Mat image = frame->image->asMat();
                cv::cvtColor(image, canvas, COLOR_RGB2BGR);
                cv::putText(canvas, ctime(&tt), Point(10, 50), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(255, 0, 0), 3);
                imshow("Image", canvas);
            }
            else {
                cout << "Frame received, timestamp = " << ctime(&tt) << endl;
            }

            incoming = false;
        }

        if (!routio::wait(20)) break;
        if (!waitKey(1) && !headless) break;
    }

    exit(0);
}
