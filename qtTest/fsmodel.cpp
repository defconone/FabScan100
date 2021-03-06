#include "fsmodel.h"

#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/features/normal_3d.h>
#include <pcl/surface/gp3.h>
#include <pcl/surface/poisson.h>

FSModel::FSModel()
{
    pointCloud.reset(new pcl::PointCloud<pcl::PointXYZRGB>);
}

void FSModel::convertPointCloudToSurfaceMesh()
{
    // Load input file into a PointCloud<T> with an appropriate type
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZ>);
    //sensor_msgs::PointCloud2 cloud_blob;
    //pcl::io::loadPCDFile ("bearHigh.pcd", cloud_blob);
    //pcl::fromROSMsg (cloud_blob, *cloud);
    //* the data should be available in cloud

    cloud->points.resize(pointCloud->size());
    for (size_t i = 0; i < pointCloud->points.size(); i++) {
        cloud->points[i].x = pointCloud->points[i].x;
        cloud->points[i].y = pointCloud->points[i].y;
        cloud->points[i].z = pointCloud->points[i].z;
    }

    // Normal estimation*
    pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> n;
    pcl::PointCloud<pcl::Normal>::Ptr normals (new pcl::PointCloud<pcl::Normal>);
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZ>);
    tree->setInputCloud (cloud);
    n.setInputCloud (cloud);
    n.setSearchMethod (tree);
    //n.setRadiusSearch(15.0);
    n.setKSearch (20);
    n.compute (*normals);
    //* normals should not contain the point normals + surface curvatures

    // Concatenate the XYZ and normal fields*
    pcl::PointCloud<pcl::PointNormal>::Ptr cloud_with_normals (new pcl::PointCloud<pcl::PointNormal>);
    pcl::concatenateFields (*cloud, *normals, *cloud_with_normals);
    //* cloud_with_normals = cloud + normals

    // Create search tree*
    pcl::search::KdTree<pcl::PointNormal>::Ptr tree2 (new pcl::search::KdTree<pcl::PointNormal>);
    tree2->setInputCloud (cloud_with_normals);

    // Initialize objects
    pcl::GreedyProjectionTriangulation<pcl::PointNormal> gp3;

    // Set the maximum distance between connected points (maximum edge length)
    gp3.setSearchRadius (15.00);

    // Set typical values for the parameters
    gp3.setMu (2.5);
    gp3.setMaximumNearestNeighbors (100);
    gp3.setMaximumSurfaceAngle(M_PI/4); // 45 degrees
    gp3.setMinimumAngle(M_PI/18); // 10 degrees
    gp3.setMaximumAngle(2*M_PI/3); // 120 degrees
    gp3.setNormalConsistency(false);

    // Get result
    gp3.setInputCloud (cloud_with_normals);
    gp3.setSearchMethod (tree2);
    gp3.reconstruct (triangles);

    // Additional vertex information
    std::vector<int> parts = gp3.getPartIDs();
    std::vector<int> states = gp3.getPointStates();

    pcl::io::savePLYFile("mesh.ply", triangles);
}

void FSModel::convertPointCloudToSurfaceMesh2()
{
    // Load input file into a PointCloud<T> with an appropriate type
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZ>);

    cloud->points.resize(pointCloud->size());
    for (size_t i = 0; i < pointCloud->points.size(); i++) {
        cloud->points[i].x = pointCloud->points[i].x;
        cloud->points[i].y = pointCloud->points[i].y;
        cloud->points[i].z = pointCloud->points[i].z;
    }

    // Normal estimation*
    pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> n;
    pcl::PointCloud<pcl::Normal>::Ptr normals (new pcl::PointCloud<pcl::Normal>);
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZ>);
    tree->setInputCloud (cloud);
    n.setInputCloud (cloud);
    n.setSearchMethod (tree);
    //n.setRadiusSearch(15.0);
    n.setKSearch (20);
    n.compute (*normals);
    //* normals should not contain the point normals + surface curvatures

    // Concatenate the XYZ and normal fields*
    pcl::PointCloud<pcl::PointNormal>::Ptr cloud_with_normals (new pcl::PointCloud<pcl::PointNormal>);
    pcl::concatenateFields (*cloud, *normals, *cloud_with_normals);


    pcl::Poisson<pcl::PointNormal> poisson;
    poisson.setDepth(9);
    poisson.setDegree(2);
    poisson.setSamplesPerNode(1);
    poisson.setScale(1.25);
    poisson.setIsoDivide(8);
    poisson.setConfidence(0);
    poisson.setManifold(0);
    poisson.setOutputPolygons(0);
    poisson.setSolverDivide(8);

    poisson.setInputCloud(cloud_with_normals);

    poisson.reconstruct (triangles);

}

void FSModel::loadPointCloud(const std::string &file_name)
{
    qDebug("Opening pcl...");
    if (pcl::io::loadPCDFile<pcl::PointXYZRGB> (file_name, *pointCloud) == -1) //* load the file
    {
        PCL_ERROR ("Couldn't read pcd file  \n");
        return ;
    }
    std::cout << "Loaded "
                << pointCloud->width * pointCloud->height
                << " data points from: "<< file_name
                << std::endl;
      FSController::getInstance()->geometries->setPointCloudTo(pointCloud);
      std::cout << "Done from Model" << std::endl;
}

void FSModel::savePointCloud(const std::string &file_name)
{
    pcl::io::savePCDFileASCII (file_name, *pointCloud);
    std::cerr << "Saved " << pointCloud->points.size() << " data points to " << file_name << std::endl;
}

void FSModel::addPointToPointCloud(FSPoint point)
{
    //qDebug()<<"added Point to cloud";
    pcl::PointXYZRGB p;
    p.x = point.x;
    p.y = point.y;
    p.z = point.z;
    int rgb = ((int)point.color.red) << 16 | ((int)point.color.green) << 8 | ((int)point.color.blue);
    p.rgb = rgb;
    pointCloud->push_back(p);
}
