# Week 1

### Progress

This week's goal was to familiarize myself with OpenCL and get some basic OpenCL code working locally on my laptop.
The problem I was dealing with was that when I selected the platform with the GPU in OpenCL (AMD Accelerated Processing)
the GPU driver was failing, causing a reboot. I took two steps to fix this.
1) I checked the current OpenGL version. My version was ```Mesa 24.3.0-devel```. To be sure I downgraded to the older version: ```Mesa 23.2.1-1ubuntu3.1~22.04.3```.

After this, I managed to successfully execute the simple A+B program with vectors of size 1000. However, upping the size still caused reboot.

2) My main version for what was causing the problem was that I didn't have enough VRAM. The fix was to launch ubuntu in TTY mode (to make sure the system takes up the least possible amount of VRAM).

As a result the program was working with every vector size I tried (the maximum was 26 000 000). 

Another problem that surfaced was that total execution time of the program was greater than that of the simple program that doesn't use parallel computing. However, the part that calculated the sum of two vectors was working faster (0.11 seconds against 0.14 seconds).

### Goals

1. I need to confirm whether it was lack of VRAM that was causing the driver failure. 
2. After I confirm this, I want to determine more precisely how much memory I can use without causing errors.
3. I will use OpenCL profiling tools to determine, why my program executes slower than the simple one.

# Week 2

### [Presentation link](https://docs.google.com/presentation/d/1yaa_cGK-k3zo7yhEbvAk6yWemQ75FB533Ybfw3aF9FA/edit?usp=sharing)


