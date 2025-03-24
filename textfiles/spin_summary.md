SPIN is aiming to solve a much broader problem than ours. And yet we can borrow some ideas from them. 

The thought that lies in foundation of their project is that while p2p is efficient, reading data from cached pages is much faster. Therefore, one shouldn't blindly use p2p for sending data to GPU memory, since if it has a lot cache hits reading it through usual, CPU mediated mechanism would be more wise.

They implemented a simple algorithm. Assume the goal is to transfer some data from the drive to the GPU memory. If all data is cached, then they just read it from there. Otherwise they split the data into contiguous segments in an optimal way, and for cached segments they read them from page cache, and for others they perform p2p DMA.

Implementation-wise, they have some interesting ideas.

1. They map GPU memory to the CPU address space (using OpenCL extension for AMD and NVIDIA library for NVIDIA).

2. For implementing p2p DMA they leveraged existing direct disk I/O interface (O_DIRECT) that allows to transfer data from drive to the system memory bypassing the page cache. To allow this interface to work with mapped GPU addresses they store the real GPU address in a dummy buffer; they use an unused flag (`arch_1`) to mark the pages that store the addresses; when the slightly modified NVMe driver gets those pages and detects the flag it extracts the real address and performs p2p.

