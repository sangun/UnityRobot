#include "Detector.hpp"

using namespace cv;
using namespace std;
using namespace robotmapping;

void Detector::processImage()
{
    Mat queryDescriptor;

    vector<KeyPoint> keypoints;
    orb->detect(currentFrame, keypoints);
	orb->compute(currentFrame, keypoints, queryDescriptor);

    vector<DMatch> matches;
    matcher->match(queryDescriptor, trainDescriptor, matches);

    DMatch *bestMatch = nullptr;
    for(auto &match : matches) {
        if(bestMatch == nullptr)
            bestMatch = &match;
        else if(match.distance < bestMatch->distance)
            bestMatch = &match;
    }

    //TODO: Label the movement blobs instead of just drawing circles
    Mat result = currentFrame.clone();
//    for(auto &match : matches)
//    {
//        circle(result, keypoints[match.imgIdx].pt, 10, Scalar(0, 255, 0), 10);
//    }

    circle(result, keypoints[bestMatch->queryIdx].pt, 10, Scalar(0, 255, 0), 10);

	imshow("Result", result);
}

Detector::Detector(const string fileName)
{
    orb = ORB::create();

    FileStorage fs2(fileName, FileStorage::READ);


    FileNode descriptorNode = fs2["Descriptors"];
    FileNode keypointsNode = fs2["Keypoints"];
    FileNode sampleNode = fs2["Sample"];

    if(descriptorNode.isNone() || keypointsNode.isNone() || sampleNode.isNone()) //We couldn't find the descriptors in the file
        throw runtime_error("Sample is not complete!");

    read(descriptorNode, trainDescriptor);
    read(keypointsNode, trainKeypoints);
    read(sampleNode, trainSample);

    fs2.release();

    cout << "Read sample from file" << endl;

    matcher = new cv::BFMatcher(cv::NORM_HAMMING, true);
}

const vector<Robot>&Detector::getRobots() const
{
    return robots;
}

void Detector::OnIncomingFrame(const Mat& frame) noexcept
{
    bufferFrame = currentFrame.clone();
    currentFrame = frame.clone();

    if(bufferFrame.empty())
        return;

    processImage();
}