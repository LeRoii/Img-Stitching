#include "jetsonEncoder.h"
#include "udp_publisher.h"

udp_publisher::UdpPublisher udp_pub;


int save_id = 0;

 size_t GetFileSize(const std::string& file_name){
	std::ifstream in(file_name.c_str());
	in.seekg(0, std::ios::end);
	size_t size = in.tellg();
	in.close();
	return size; //单位是：Byte
}

static int write_encoder_output_frame(ofstream * stream, NvBuffer * buffer)
{
    stream->write((char *) buffer->planes[0].data, buffer->planes[0].bytesused);
    udp_pub.sendimage(buffer->planes[0].data,buffer->planes[0].bytesused);
    // char *socketData;
    // udp_pub.getSocketData(socketData);
    // printf("get Data: %s\n",socketData);
    return 0;
}

/**
 * Callback function called after capture plane dqbuffer of NvVideoEncoder class.
 * See NvV4l2ElementPlane::dqThread() in sample/common/class/NvV4l2ElementPlane.cpp
 * for details.
 *
 * @param v4l2_buf       : dequeued v4l2 buffer
 * @param buffer         : NvBuffer associated with the dequeued v4l2 buffer
 * @param shared_buffer  : Shared NvBuffer if the queued buffer is shared with
 *                         other elements. Can be NULL.
 * @param arg            : private data set by NvV4l2ElementPlane::startDQThread()
 *
 * @return               : true for success, false for failure (will stop DQThread)
 */
bool
encoder_capture_plane_dq_callback(struct v4l2_buffer *v4l2_buf, NvBuffer * buffer,
                                  NvBuffer * shared_buffer, void *arg)
{
    char str[80];
    size_t saved_size;
    context_t *ctx = (context_t *) arg;
    NvVideoEncoder *enc = ctx->enc;

    if (!v4l2_buf)
    {
        cerr << "Failed to dequeue buffer from encoder capture plane" << endl;
        //abort(ctx);
        return false;
    }
    printf("encoder_capture_plane_dq_callback,size:%d\n",buffer->planes[0].bytesused);


    saved_size = GetFileSize(ctx->out_file_path);
    std::cout<<"^^^^^^^^^^^^^^^^^^^^^^^^^^^^size: "<<saved_size<<std::endl;

    if(saved_size>104857600){ //100MB = 104857600B
        save_id ++;
        sprintf(str, "/home/nvidia/out_%d.h264", save_id);
        ctx->out_file_path = str;
        std::cout<<"The video save to a new file!"<<std::endl;
        ctx->out_file = new ofstream(ctx->out_file_path);
        TEST_ERROR(!ctx->out_file->is_open(), "Could not open output file");
    }

    write_encoder_output_frame(ctx->out_file, buffer);

    /* qBuffer on the capture plane */
    if (enc->capture_plane.qBuffer(*v4l2_buf, NULL) < 0)
    {
        cerr << "Error while Qing buffer at capture plane" << endl;
        //abort(ctx);
        return false;
    }

    /* GOT EOS from encoder. Stop dqthread. */
    if (buffer->planes[0].bytesused == 0)
    {
        return false;
    }

    return true;
}

jetsonEncoder::jetsonEncoder()
{
    int ret = 0;
    frame_count = 0;
    set_defaults(&ctx);
    ctx.out_file_path = "/home/nvidia/out.h264";

    ctx.encoder_pixfmt = V4L2_PIX_FMT_H264;

    ctx.enc = NvVideoEncoder::createVideoEncoder("enc0");
    TEST_ERROR(!ctx.enc, "Could not create encoder");

    ctx.out_file = new ofstream(ctx.out_file_path);
    TEST_ERROR(!ctx.out_file->is_open(), "Could not open output file");

    /**
     * It is necessary that Capture Plane format be set before Output Plane
     * format.
     * It is necessary to set width and height on the capture plane as well.
     */
    ret = ctx.enc->setCapturePlaneFormat(ctx.encoder_pixfmt, ctx.width,
                                      ctx.height, 2 * 1024 * 1024);
    TEST_ERROR(ret < 0, "Could not set capture plane format");

    ret = ctx.enc->setOutputPlaneFormat(V4L2_PIX_FMT_YUV420M, ctx.width,
                                      ctx.height);
    TEST_ERROR(ret < 0, "Could not set output plane format");

    ret = ctx.enc->setBitrate(ctx.bitrate);
    TEST_ERROR(ret < 0, "Could not set bitrate");

    ret = ctx.enc->setProfile(V4L2_MPEG_VIDEO_H264_PROFILE_HIGH);
    TEST_ERROR(ret < 0, "Could not set encoder profile");

    ret = ctx.enc->setLevel(V4L2_MPEG_VIDEO_H264_LEVEL_5_0);
    TEST_ERROR(ret < 0, "Could not set encoder level");

    ret = ctx.enc->setFrameRate(ctx.fps_n, ctx.fps_d);
    TEST_ERROR(ret < 0, "Could not set framerate");

    /**
     * Query, Export and Map the output plane buffers so that we can read
     * raw data into the buffers
     */
    ret = ctx.enc->output_plane.setupPlane(V4L2_MEMORY_MMAP, 10, true, false);
    TEST_ERROR(ret < 0, "Could not setup output plane");

    /**
     * Query, Export and Map the capture plane buffers so that we can write
     * encoded data from the buffers
     */
    ret = ctx.enc->capture_plane.setupPlane(V4L2_MEMORY_MMAP, 6, true, false);
    TEST_ERROR(ret < 0, "Could not setup capture plane");

    /* output plane STREAMON */
    ret = ctx.enc->output_plane.setStreamStatus(true);
    TEST_ERROR(ret < 0, "Error in output plane streamon");

    /* capture plane STREAMON */
    ret = ctx.enc->capture_plane.setStreamStatus(true);
    TEST_ERROR(ret < 0, "Error in capture plane streamon");

    ctx.enc->capture_plane.setDQThreadCallback(encoder_capture_plane_dq_callback);

    /**
     * startDQThread starts a thread internally which calls the
     * encoder_capture_plane_dq_callback whenever a buffer is dequeued
     * on the plane
     */
    ctx.enc->capture_plane.startDQThread(&ctx);

    /* Enqueue all the empty capture plane buffers */
    for (uint32_t i = 0; i < ctx.enc->capture_plane.getNumBuffers(); i++)
    {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane planes[MAX_PLANES];

        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, MAX_PLANES * sizeof(struct v4l2_plane));

        v4l2_buf.index = i;
        v4l2_buf.m.planes = planes;

        ret = ctx.enc->capture_plane.qBuffer(v4l2_buf, NULL);
        if (ret < 0)
        {
            cerr << "Error while queueing buffer at capture plane" << endl;
            //abort(&ctx);
            //goto cleanup;
        }
    }

    
}

jetsonEncoder::~jetsonEncoder()
{
    if(ctx.enc)
        delete ctx.enc;
}

void jetsonEncoder::set_defaults(context_t * ctx)
{
    memset(ctx, 0, sizeof(context_t));

    ctx->bitrate = 4 * 1024* 1024;
    ctx->fps_n = 30;
    ctx->fps_d = 1;
    ctx->width = 1920;
    ctx->height = 720;
}

void jetsonEncoder::copyYuvToBuffer(uint8_t *yuv_bytes, NvBuffer &buffer)
{

    uint32_t i, j, k;
    uint8_t *src = yuv_bytes;
    uint8_t *dst;


    for (i = 0; i < buffer.n_planes; i++) {

        NvBuffer::NvBufferPlane &plane = buffer.planes[i];
        uint32_t bytes_per_row = plane.fmt.bytesperpixel * plane.fmt.width;
        dst = plane.data;
        plane.bytesused = 0;

        for (j = 0; j < plane.fmt.height; j++) {
            for (k = 0; k < bytes_per_row; k++) {
                *dst = *src;
                dst++;
                src++;
            }
            dst += (plane.fmt.stride - bytes_per_row);
        }
        plane.bytesused = plane.fmt.stride * plane.fmt.height;
    }
}

int jetsonEncoder::encodeFrame(uint8_t *yuv_bytes)
{
    int ret = 0;
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[MAX_PLANES];
    NvBuffer *buffer = ctx.enc->output_plane.getNthBuffer(frame_count);

    memset(&v4l2_buf, 0, sizeof(v4l2_buf));
    memset(planes, 0, MAX_PLANES * sizeof(struct v4l2_plane));

    v4l2_buf.m.planes = planes;

    if (frame_count < ctx.enc->output_plane.getNumBuffers()) {
        v4l2_buf.index = frame_count;

        if (yuv_bytes)
            copyYuvToBuffer(yuv_bytes, *buffer);
        else
            v4l2_buf.m.planes[0].bytesused = 0;

    } else {
        if (ctx.enc->output_plane.dqBuffer(v4l2_buf, &buffer, nullptr, 10) < 0) {
            cerr << "ERROR while DQing buffer at output plane" << endl;
            //abort(&ctx);
            //return cleanup(ctx, 1);
        }
    }

    
    if (frame_count >= ctx.enc->output_plane.getNumBuffers()) {
        if (yuv_bytes)
            copyYuvToBuffer(yuv_bytes, *buffer);
        else
            v4l2_buf.m.planes[0].bytesused = 0;
    }

    ret = ctx.enc->output_plane.qBuffer(v4l2_buf, nullptr);
    if (ret < 0) {
        cerr << "Error while queueing buffer at output plane" << endl;
        //abort(&ctx);
        //return cleanup(ctx, 1);
    }


    frame_count++;
    return ret;
}


 int jetsonEncoder::pubTargetData(targetInfo  target_data){

    int needSendlen = sizeof(target_data);
    cerr<<"send Data length:"<<needSendlen<<endl;
    udp_pub.sendData(target_data,needSendlen);
 }

 controlData  jetsonEncoder::getControlData(){
    controlData ctl_data;
    ctl_data = udp_pub.recvData();
    return ctl_data;
 }
 


