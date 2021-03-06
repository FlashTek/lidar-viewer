/*
MIT License

Copyright(c) 2018 Roland Zimmermann

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../OGLW/window.h"
#include "../OGLW/shader.h"
#include "../OGLW/camera.h"

#include "main_window.h"
#include "point_viewer_scene.h"
#include "surface_viewer_scene.h"
#include "utility.h"

#ifdef USE_PCL
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/features/normal_3d.h>
#include <pcl/surface/gp3.h>
#include <pcl/surface/marching_cubes_hoppe.h>
#include <pcl/surface/poisson.h>
#include <pcl/filters/passthrough.h>
#include <pcl/surface/mls.h>
#endif

#ifdef USE_PCL
boost::shared_ptr<ColoredMesh> pcl_test_rgb(const char* filename)
{
	auto cloud = utility::loadPointCloid<pcl::PointXYZRGB>(filename);

	pcl::search::KdTree<pcl::PointXYZRGB>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGB>);
	pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud_with_normals(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
	// Init object (second point type is for the normals, even if unused)
	pcl::MovingLeastSquares<pcl::PointXYZRGB, pcl::PointXYZRGBNormal> mls;

	mls.setComputeNormals(true);

	// Set parameters
	mls.setInputCloud(cloud);
	mls.setPolynomialFit(true);
	mls.setSearchMethod(tree);
	mls.setSearchRadius(0.3);
	mls.setUpsamplingMethod(pcl::MovingLeastSquares<pcl::PointXYZRGB, pcl::PointXYZRGBNormal>::UpsamplingMethod::SAMPLE_LOCAL_PLANE);
	mls.setUpsamplingRadius(0.25);
	mls.setUpsamplingStepSize(0.1);

	mls.process(*cloud_with_normals);

	//* normals should not contain the point normals + surface curvatures

	//* cloud_with_normals = cloud + normals

	// Create search tree*
	pcl::search::KdTree<pcl::PointXYZRGBNormal>::Ptr tree2(new pcl::search::KdTree<pcl::PointXYZRGBNormal>);
	tree2->setInputCloud(cloud_with_normals);

	pcl::PolygonMesh triangles;


	for (size_t i = 0; i < cloud_with_normals->size(); ++i)
	{
		cloud_with_normals->points[i].normal_x *= -1;
		cloud_with_normals->points[i].normal_y *= -1;
		cloud_with_normals->points[i].normal_z *= -1;
	}

	pcl::MarchingCubesHoppe<pcl::PointXYZRGBNormal> mc;
	mc.setIsoLevel(0.001);
	mc.setGridResolution(500, 500, 500);
	mc.setInputCloud(cloud_with_normals);
	mc.reconstruct(triangles);

	return utility::convertPolygonMeshToTriangleMesh(triangles, cloud_with_normals);
}

#endif
int main()
{
	bool surface = false;
	std::string filename;

	std::cout << "Filename of the point cloud data: ";
	std::cin >> filename;

#ifdef USE_PCL
	std::cout << "Use surface reconstruction?: ";
	std::cin >> surface;
#endif

	// hide the console
	// close stdin, stdout, stderr to be able to free the console
	_fcloseall();
	FreeConsole();

	// init the renderer
	OGLW::init(4, 6);

	// enable anti aliasing
	glfwWindowHint(GLFW_SAMPLES, 4);
	MainWindow window;

	if (surface)
	{
#ifdef USE_PCL
		boost::shared_ptr<std::vector<ColoredVertex>> vertices(new std::vector<ColoredVertex>());
		boost::shared_ptr<std::vector<unsigned int>> indices(new std::vector<unsigned int>());
		boost::shared_ptr<ColoredMesh> pMesh = pcl_test_rgb(filename.c_str());

		SurfaceViewerScene scene(&window, pMesh);
		window.run();
		return;
#else
		std::cout << "Program has been built without pcl support. Therefore, surface reconstruction is disabled." << std::endl;
#endif
	}
	else
	{
		int nPoints = 1000 * 100;

		boost::shared_ptr<std::vector<float>> pointData = utility::loadRawData(filename.c_str(), nPoints);
		PointViewerScene scene(&window, *pointData);
		window.run();
	}

	return 0;
}