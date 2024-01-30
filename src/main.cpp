#include <iostream>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>



/* READ IMAGES */
cv::Mat img, imgResize, imgGray, imgBlur, imgCanny, imgThre, imgDil, imgWarp, imgCrop;
std::vector<cv::Point> initialPoints, docPoints;
float w = 3*200, h = 4*200;

cv::Mat static preProcessing(cv::Mat imgResize) {
	cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	
	cv::cvtColor(imgResize, imgGray, cv::COLOR_BGR2GRAY);
	cv::GaussianBlur(imgGray, imgBlur, cv::Size(9, 9), 1, 0);
	cv::Canny(imgBlur, imgCanny, 50, 150);
	cv::dilate(imgCanny, imgDil, kernel);

	return imgDil;
}

static std::vector<cv::Point> getContours(cv::Mat imgDil) {
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;

	findContours(imgDil, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	std::vector<std::vector<cv::Point>> conPoly(contours.size());
	std::vector<cv::Rect> boundRect(contours.size());

	std::vector<cv::Point> biggestRect;
	int maxArea = 0;

	for (int i = 0; i < contours.size(); i++) {
		int area = cv::contourArea(contours[i]);
		//std::cout << area << std::endl;

		if (area > 5000) {
			float peri = cv::arcLength(contours[i], true);
			cv::approxPolyDP(contours[i], conPoly[i], 0.02 * peri, true);
			//cv::drawContours(imgResize, conPoly, i, cv::Scalar(255, 0, 255), 2);

		}

		if (area > maxArea && conPoly[i].size() == 4) {
			
			maxArea = area;
			biggestRect = { conPoly[i][0], conPoly[i][1], conPoly[i][2], conPoly[i][3] };
		}
	}
	return biggestRect;
}

static void drawPoints(std::vector<cv::Point> points, cv::Scalar color) {
	for (int i = 0; i < points.size(); i++) {
		cv::circle(imgResize, points[i], 10, color, cv::FILLED);
		cv::putText(imgResize, std::to_string(i), points[i], cv::FONT_HERSHEY_PLAIN, 5, color, 2);
	}
}

static std::vector<cv::Point> reorder(std::vector<cv::Point> initialPoints) {
	std::vector<cv::Point> newPoints;
	std::vector<int> sumCoords, subCoords;

	for (int i = 0; i < initialPoints.size(); i++) {
		sumCoords.push_back(initialPoints[i].x + initialPoints[i].y);
		subCoords.push_back(initialPoints[i].x - initialPoints[i].y);

	}
	newPoints.push_back(initialPoints[std::min_element(sumCoords.begin(), sumCoords.end()) - sumCoords.begin()]);  // 0
	newPoints.push_back(initialPoints[std::max_element(subCoords.begin(), subCoords.end()) - subCoords.begin()]);  // 1
	newPoints.push_back(initialPoints[std::min_element(subCoords.begin(), subCoords.end()) - subCoords.begin()]);  // 2
	newPoints.push_back(initialPoints[std::max_element(sumCoords.begin(), sumCoords.end()) - sumCoords.begin()]);  // 3
	return newPoints;
}

static cv::Mat getWarp(cv::Mat imgResize, std::vector<cv::Point> docPoints, float w, float h) {
	cv::Point2f src[4] = { docPoints[0], docPoints[1], docPoints[2], docPoints[3]};
	cv::Point2f dst[4] = { {0.0f, 0.0f}, {w, 0.0f}, {0.0f, h}, {w, h} };

	cv::Mat matrix = cv::getPerspectiveTransform(src, dst);
	cv::warpPerspective(imgResize, imgWarp, matrix, cv::Point(w, h));

	return imgWarp;
}

int main() {

	std::cout << "Hello, scanner!" << std::endl;

	std::string path = "Resources/3.jpg";
	img = cv::imread(path);
	//cv::resize(img, imgResize, cv::Size(), 0.3, 0.3);
	
	// Preprocessing
	imgThre = preProcessing(img);
	// Get contours  Find the biggest rectangle in the image
	initialPoints = getContours(imgThre);
	docPoints = reorder(initialPoints);
	//drawPoints(docPoints, cv::Scalar(0, 200, 0));
	
	// Warps
	imgWarp = getWarp(img, docPoints, w, h);

	// Crop
	cv::Rect roi(10, 10, w - 10 * 2, h - 10 * 2);
	imgCrop = imgWarp(roi);

	//cv::imshow("Original image", imgResize);
	//cv::imshow("Threshold image", imgThre);
	//cv::imshow("Warp image", imgWarp);
	cv::imshow("Warp image", imgCrop);
	cv::waitKey(0);

	return 0;
}