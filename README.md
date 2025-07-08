# GPU RAID

Research project exploring RAID implementation on GPU using OpenCL.

## Current Status

### Completed
- Basic OpenCL code running on AMD GPU
- Successfully executed vector addition with sizes up to 26,000,000 elements
- Resolved driver stability issues

### Problems Solved

1. **GPU Driver Crashes**
   - Problem: GPU driver failure causing system reboot when selecting AMD Accelerated Processing platform
   - Solution: Downgraded Mesa from 24.3.0-devel to 23.2.1-1ubuntu3.1~22.04.3

2. **VRAM Limitations**
   - Problem: Crashes when processing large data arrays
   - Solution: Launch Ubuntu in TTY mode to minimize system VRAM usage

### Performance Observations
- Vector sum calculation on GPU: 0.11 seconds
- Vector sum calculation on CPU: 0.14 seconds
- Total program execution time still slower than non-parallel version

## Next Steps

- [ ] Confirm that lack of VRAM was causing driver failures
- [ ] Determine precise memory limits for stable operation
- [ ] Use OpenCL profiling tools to identify performance bottlenecks

## Requirements

- OpenCL-capable GPU (tested on AMD)
- Ubuntu Linux
- Mesa drivers (version 23.2.1 recommended)

## Presentation
- Presentation could be found [here](https://docs.google.com/presentation/d/1yaa_cGK-k3zo7yhEbvAk6yWemQ75FB533Ybfw3aF9FA/edit?usp=sharing)
