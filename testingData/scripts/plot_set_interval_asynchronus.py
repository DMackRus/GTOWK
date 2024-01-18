import matplotlib.pyplot as plt
import numpy as np
from numpy import genfromtxt
import csv
import os

def main():

    # load the data
    file_count = count_files_in_directory("../walker")
    
    # data = np.zeros(file_count)
    # Load all the data
    all_data = []
    for i in range(file_count):
        file_path = "../walker/walker_setInterval_" + str(i+1) + "_testingData.csv"
        print(file_path)
        data = load_data_from_path(file_path)

        # drop top row as its header which are converted to Nan
        data = data[0]
        data = data[1:]
        all_data.append(data)

    # Loop through all data and compute mean and standard deviation information
    mean_final_cost = []
    mean_opt_time = []
    mean_percent_derivs = []

    std_deviation = []
    for i in range(file_count):
        mean_, std_deviation_ = compute_mean_and_std_deviation(all_data[i])
        mean_final_cost.append(mean_[0])
        mean_opt_time.append(mean_[1])
        mean_percent_derivs.append(mean_[2])
        # mean.append(mean_)
        # std_deviation.append(std_deviation_)

    plt.figure(figsize=(8, 12))  # Adjust the figure size as needed

    # Plot the first line
    plt.subplot(3, 1, 1)
    plt.plot(mean_final_cost, label='Final costs')
    # plt.title('Plot 1')
    plt.legend()

    # Plot the second line
    plt.subplot(3, 1, 2)
    plt.plot(mean_opt_time, label = 'Optimisation times (ms)')
    # plt.title('Plot 2')
    plt.legend()

    # Plot the third line
    plt.subplot(3, 1, 3)
    plt.plot(mean_percent_derivs , label='Mean percent derivatives')
    # plt.title('Plot 3')
    plt.legend()

    # Adjust layout to prevent overlapping
    plt.tight_layout()

    plt.show()


    # print(mean_final_cost)
    # plt.plot(mean_final_cost)
    # plt.show()

    # plt.plot(mean_opt_time)
    # plt.show()

    # plt.plot(mean_percent_derivs)
    # plt.show()

    # 6 elements in data, final cost, opt time, % derivs, time derivs, time bp, time fp


def compute_mean_and_std_deviation(data):
    size = len(data[1])
    mean = np.zeros(size)
    std_deviation = np.zeros(size)

    for i in range(size):
        mean[i] = np.mean(data[:,i])
        std_deviation[i] = np.std(data[:,i])

    return mean, std_deviation

def load_data_from_path(file_path):
    data = np.array([genfromtxt(file_path, delimiter = ',')])

    return data

def count_files_in_directory(directory_path):
    try:
        # List all files in the directory
        files = os.listdir(directory_path)

        # Count the number of files
        file_count = len(files)

        print(f'The number of files in {directory_path} is: {file_count}')

    except FileNotFoundError:
        print(f'The directory {directory_path} does not exist.')

    return file_count


if __name__ == "__main__":
    main()