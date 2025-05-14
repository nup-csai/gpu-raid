import numpy as np
def generate_test_data(filename, size=26000000):
    """
    Generate test data file with random float numbers
    Args:
    filename (str): Output filename
    size (int): Number of float values to generate
    """
    data = np.random.uniform(-10.0, 10.0, size=size)
    np.savetxt(filename, data, fmt='%.6f')

generate_test_data('data/vector1.dat')
generate_test_data('data/vector2.dat')
