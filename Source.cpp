#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <chrono>

using namespace std;
using namespace std::chrono;

struct Point {
    double x;
    double y;
};

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int num_procs, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Initialize random seed
    srand(time(NULL));

    // Generate random data points
    vector<Point> points;
    const int num_points = 100000;
    const int num_dims = 2;
    for (int i = 0; i < num_points / num_procs; ++i) {
        Point p;
        p.x = rand() % 100;
        p.y = rand() % 100;
        points.push_back(p);
    }

    // Broadcast the data to all processes
    MPI_Bcast(points.data(), num_points / num_procs * num_dims, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Initialize the centroids randomly
    vector<Point> centroids;
    const int k = 4;
    if (rank == 0) {
        for (int i = 0; i < k; ++i) {
            Point p;
            p.x = rand() % 100;
            p.y = rand() % 100;
            centroids.push_back(p);
        }
    }
    MPI_Bcast(centroids.data(), k * num_dims, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    auto start = high_resolution_clock::now();
    // Run the K-Means algorithm
    const int max_iterations = 10000;
    for (int iteration = 0; iteration < max_iterations; ++iteration) {
        // Assign each point to the closest centroid
        vector<int> assignments(points.size());
        for (int i = 0; i < points.size(); ++i) {
            double min_distance = INFINITY;
            for (int j = 0; j < centroids.size(); ++j) {
                double distance = sqrt(pow(points[i].x - centroids[j].x, 2) + pow(points[i].y - centroids[j].y, 2));
                if (distance < min_distance) {
                    min_distance = distance;
                    assignments[i] = j;
                }
            }
        }

        // Compute the new centroids
        vector<Point> new_centroids(k);
        vector<int> counts(k);
        for (int i = 0; i < points.size(); ++i) {
            int cluster = assignments[i];
            new_centroids[cluster].x += points[i].x;
            new_centroids[cluster].y += points[i].y;
            counts[cluster]++;
        }
        for (int i = 0; i < k; ++i) {
            if (counts[i] > 0) {
                new_centroids[i].x /= counts[i];
                new_centroids[i].y /= counts[i];
            }
        }

        // Check for convergence
        bool converged = true;
        for (int i = 0; i < k; ++i) {
            if (sqrt(pow(new_centroids[i].x - centroids[i].x, 2) + pow(new_centroids[i].y - centroids[i].y, 2)) > 1e-6) {
                converged = false;
                break;
            }
        }
        if (converged)
            break;

        // Update the centroids
        centroids = new_centroids;
        MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, centroids.data(), k * num_dims, MPI_DOUBLE, MPI_COMM_WORLD);
    }
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();
    // Print the final centroids
    if (rank == 0) {
        // Print the duration in milliseconds
        cout << "Duration: " << duration << " microseconds" << endl;
        cout << "Final centroids:" << endl;
        for (int i = 0; i < k; ++i) {
            cout << centroids[i].x << "," << centroids[i].y << endl;
        }
    }

    MPI_Finalize();
    return 0;
}