#ifndef YUYV2RGB_CUH
	#define YUYV2RGB_CUH
	extern "C" void yuyv2rgb_cuda(char *YUV, char *RGB, int num_blocks, int block_size);
#endif
