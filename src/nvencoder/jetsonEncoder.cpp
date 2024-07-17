#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "jetsonEncoder.h"
#include "udp_publisher.h"
#include <chrono>
#include <iomanip>
#include "spdlog/spdlog.h"
#include "stitcherglobal.h"

static int g_portNum;
udp_publisher::UdpPublisher udp_pub;
// typedef websocketpp::client<websocketpp::config::asio_client> client;
typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// client c;
// client::connection_ptr con;
websocketpp::connection_hdl g_hdl;
server m_endpoint;

    
int save_id = 0;

static bool webSocketSendStart = false;

 size_t GetFileSize(const std::string& file_name){
	std::ifstream in(file_name.c_str());
	in.seekg(0, std::ios::end);
	size_t size = in.tellg();
	in.close();
	return size; //单位是：Byte
}

static void on_open(websocketpp::connection_hdl hdl)
{
    g_hdl = hdl;
    std::string msg = "send started";
    // m_endpoint.send(g_hdl, msg, websocketpp::frame::opcode::text);
    // c->get_alog().write(websocketpp::log::alevel::app, "Tx: " + msg);
    webSocketSendStart = true;
}


static void startServerTh()
{
    m_endpoint.set_error_channels(websocketpp::log::elevel::all);
    m_endpoint.set_access_channels(websocketpp::log::alevel::devel);
    m_endpoint.clear_access_channels(websocketpp::log::alevel::frame_payload);
    m_endpoint.init_asio();
    m_endpoint.set_open_handler(std::bind(&on_open, std::placeholders::_1));
    m_endpoint.listen(g_portNum);
    m_endpoint.start_accept();
    // 开始Asio事件循环
    m_endpoint.run();
}

static std::string base64Decode(const char* Data, int DataByte) {
	//解码表
	const char DecodeTable[] =
	{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		62, // '+'
		0, 0, 0,
		63, // '/'
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, // '0'-'9'
		0, 0, 0, 0, 0, 0, 0,
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
		13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, // 'A'-'Z'
		0, 0, 0, 0, 0, 0,
		26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
		39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, // 'a'-'z'
	};
	std::string strDecode;
	int nValue;
	int i = 0;
	while (i < DataByte) {
		if (*Data != '\r' && *Data != '\n') {
			nValue = DecodeTable[*Data++] << 18;
			nValue += DecodeTable[*Data++] << 12;
			strDecode += (nValue & 0x00FF0000) >> 16;
			if (*Data != '=') {
				nValue += DecodeTable[*Data++] << 6;
				strDecode += (nValue & 0x0000FF00) >> 8;
				if (*Data != '=') {
					nValue += DecodeTable[*Data++];
					strDecode += nValue & 0x000000FF;
				}
			}
			i += 4;
		}
		else {
			Data++;
			i++;
		}
	}
	return strDecode;
}
 
 
static std::string base64Encode(const unsigned char* Data, int DataByte) {
	//编码表
	const char EncodeTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	//返回值
	std::string strEncode;
	unsigned char Tmp[4] = { 0 };
	int LineLength = 0;
	for (int i = 0; i < (int)(DataByte / 3); i++) {
		Tmp[1] = *Data++;
		Tmp[2] = *Data++;
		Tmp[3] = *Data++;
		strEncode += EncodeTable[Tmp[1] >> 2];
		strEncode += EncodeTable[((Tmp[1] << 4) | (Tmp[2] >> 4)) & 0x3F];
		strEncode += EncodeTable[((Tmp[2] << 2) | (Tmp[3] >> 6)) & 0x3F];
		strEncode += EncodeTable[Tmp[3] & 0x3F];
		if (LineLength += 4, LineLength == 76) { strEncode += "\r\n"; LineLength = 0; }
	}
	//对剩余数据进行编码
	int Mod = DataByte % 3;
	if (Mod == 1) {
		Tmp[1] = *Data++;
		strEncode += EncodeTable[(Tmp[1] & 0xFC) >> 2];
		strEncode += EncodeTable[((Tmp[1] & 0x03) << 4)];
		strEncode += "==";
	}
	else if (Mod == 2) {
		Tmp[1] = *Data++;
		Tmp[2] = *Data++;
		strEncode += EncodeTable[(Tmp[1] & 0xFC) >> 2];
		strEncode += EncodeTable[((Tmp[1] & 0x03) << 4) | ((Tmp[2] & 0xF0) >> 4)];
		strEncode += EncodeTable[((Tmp[2] & 0x0F) << 2)];
		strEncode += "=";
	}
 
 
	return strEncode;
}
 
//imgType 包括png bmp jpg jpeg等opencv能够进行编码解码的文件
static std::string Mat2Base64(const cv::Mat &img, std::string imgType) {
	//Mat转base64
	std::string img_data;
	std::vector<uchar> vecImg;
	std::vector<int> vecCompression_params;
	vecCompression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
	vecCompression_params.push_back(90);
	imgType = "." + imgType;
	cv::imencode(imgType, img, vecImg, vecCompression_params);
	img_data = base64Encode(vecImg.data(), vecImg.size());
	return img_data;
}
 
 
static cv::Mat Base2Mat(std::string &base64_data) {
	cv::Mat img;
	std::string s_mat;
	s_mat = base64Decode(base64_data.data(), base64_data.size());
	std::vector<char> base64_img(s_mat.begin(), s_mat.end());
	img = cv::imdecode(base64_img, CV_LOAD_IMAGE_COLOR);
	return img;
}



static int write_encoder_output_frame(ofstream * stream, NvBuffer * buffer)
{
    stream->write((char *) buffer->planes[0].data, buffer->planes[0].bytesused);
    // udp_pub.sendimage(buffer->planes[0].data,buffer->planes[0].bytesused);
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
    spdlog::debug("encoder_capture_plane_dq_callback,size:{}",buffer->planes[0].bytesused);


    // saved_size = GetFileSize(ctx->out_file_path);
    // spdlog::debug("^^^^^^^^^^^^^^^^^^^^^^^^^^^^size:{} ",saved_size);

    // if(saved_size>104857600 || !ctx->out_file->is_open()){ //100MB = 104857600B
    //     save_id ++;
    //     sprintf(str, "/home/nvidia/out_%d.h264", save_id);

    //     std::time_t tt = std::chrono::system_clock::to_time_t (std::chrono::system_clock::now());
    //     std::stringstream ss;
    //     ss << std::put_time(std::localtime(&tt), "%F-%H-%M-%S");
    //     std::string str = "/home/nvidia/"+ss.str()+".h264";
    //     ss.str("");
    //     ss << str;
    //     ss >> ctx->out_file_path;
    //     // ctx->out_file_path = str.c_str();

    //     // ctx->out_file_path = str;
    //     spdlog::debug("The video save to a new file!");
    //     ctx->out_file = new ofstream(ctx->out_file_path);
    //     TEST_ERROR(!ctx->out_file->is_open(), "Could not open output file");
    // }

    // write_encoder_output_frame(ctx->out_file, buffer);
    //  cv::Mat org;
    
    // m_h264Decoder.decode(buffer->planes[0].data, buffer->planes[0].bytesused, org);
    // if(!org.empty())
    // {
    //     cv::imshow("org",org);
    //     cv::waitKey(10);
    // }
    // c.send(hdl, buffer->planes[0].data, buffer->planes[0].bytesused, websocketpp::frame::opcode::binary);
    if(webSocketSendStart)
    {
        // write_encoder_output_frame(ctx->out_file, buffer);
        // int img = open("/home/nvidia/2022-08-16-19-32-27.h264", O_RDONLY);
        // if(img == -1)
        //     printf("Failed to open file for rendering\n");
        // int imgbufsize = 30000;
        // unsigned char *imgbuf = (unsigned char*)malloc(imgbufsize);
        // int cnt = read(img, imgbuf, imgbufsize);
        //     printf("read %d bytes\n", cnt);
        // cv::Mat org;
        // m_h264Decoder.initial();
        // m_h264Decoder.decode(buffer->planes[0].data, buffer->planes[0].bytesused, org);
        // if(!org.empty())
        // {
        //     cv::imshow("org",org);
        //     cv::waitKey(10);
        // }
        try{
            m_endpoint.send(g_hdl, buffer->planes[0].data, buffer->planes[0].bytesused, websocketpp::frame::opcode::binary);

        }
        catch(...)
        {
            spdlog::warn("websocket connection failed");
            webSocketSendStart = false;

            // enc->capture_plane.stopDQThread();
            // enc->output_plane.setStreamStatus(false);
            // enc->capture_plane.setStreamStatus(false);

            // enc->output_plane.setStreamStatus(true);
            // enc->capture_plane.setStreamStatus(true);

            // enc->capture_plane.startDQThread(ctx);

            // spdlog::warn("enc->output_plane.getNumBuffers():{}", enc->output_plane.getNumBuffers());
        }
                // m_endpoint.send(g_hdl, imgbuf, cnt, websocketpp::frame::opcode::binary);

        // for(int i=0;i<20;i++)
        // {
        //     printf("%#x, ", buffer->planes[0].data[i]);
        // }

        // webSocketSendStart = false;
    }
    /* qBuffer on the capture plane */
    if(webSocketSendStart)
    {
        if (enc->capture_plane.qBuffer(*v4l2_buf, NULL) < 0)
        {
            cerr << "Error while Qing buffer at capture plane" << endl;
            //abort(ctx);
            return false;
        }
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

    std::time_t tt = std::chrono::system_clock::to_time_t (std::chrono::system_clock::now());
    std::stringstream ss;
    ss << std::put_time(std::localtime(&tt), "%F-%H-%M-%S");
    std::string str = "/home/nvidia/"+ss.str()+".h264";
    ss.str("");
    ss << str;
    ss >> ctx.out_file_path;

    spdlog::debug("ctx.out_file_path:{}", ctx.out_file_path);

    // ctx.out_file_path = "/home/nvidia/"+str+".h264";

    ctx.encoder_pixfmt = V4L2_PIX_FMT_H264;

    ctx.enc = NvVideoEncoder::createVideoEncoder("enc0");
    TEST_ERROR(!ctx.enc, "Could not create encoder");

    // ctx.out_file = new ofstream(ctx.out_file_path);
    // TEST_ERROR(!ctx.out_file->is_open(), "Could not open output file");

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

    // websocket init
    
    // std::string uri = "ws://localhost:9002";

    // c.set_access_channels(websocketpp::log::alevel::all);
    // c.clear_access_channels(websocketpp::log::alevel::frame_payload);
    // c.clear_access_channels(websocketpp::log::alevel::frame_header);

    // c.init_asio();

    // websocketpp::lib::error_code ec;
    // con = c.get_connection(weburi, ec);
    // con->add_subprotocol("janus-protocol");
    // if(ec)
    // {
    //     spdlog::warn("could not create connection because:{}", ec.message());
    // }

    // hdl = con->get_handle();
    // c.connect(con);
    // std::thread th(&client::run, &c);
    // th.detach();
    // sleep(3);
    // c.send(hdl, "hello", websocketpp::frame::opcode::text);
    // c.close(hdl, websocketpp::close::status::normal, "");
}

jetsonEncoder::jetsonEncoder(bool on, int portnum):websocketOn(on)
{
    g_portNum = portnum;
    int ret = 0;
    frame_count = 0;
    set_defaults(&ctx);

    std::time_t tt = std::chrono::system_clock::to_time_t (std::chrono::system_clock::now());
    std::stringstream ss;
    ss << std::put_time(std::localtime(&tt), "%F-%H-%M-%S");
    std::string str = "/home/nvidia/"+ss.str()+".h264";
    ss.str("");
    ss << str;
    ss >> ctx.out_file_path;

    spdlog::debug("ctx.out_file_path:{}", ctx.out_file_path);

    // ctx.out_file_path = "/home/nvidia/"+str+".h264";

    ctx.encoder_pixfmt = V4L2_PIX_FMT_H264;

    ctx.enc = NvVideoEncoder::createVideoEncoder("enc0");
    TEST_ERROR(!ctx.enc, "Could not create encoder");

    // ctx.out_file = new ofstream(ctx.out_file_path);
    // TEST_ERROR(!ctx.out_file->is_open(), "Could not open output file");

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

    ret = ctx.enc->setIFrameInterval(25);
    ret = ctx.enc->setIDRInterval(30);  //ok
    ret = ctx.enc->setInsertSpsPpsAtIdrEnabled(true);
    TEST_ERROR(ret < 0, "Could not set setIFrameInterval");

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

    // websocket  client init
    
    // if(websocketOn)
    // {
    //     c.set_access_channels(websocketpp::log::alevel::all);
    //     c.clear_access_channels(websocketpp::log::alevel::frame_payload);
    //     c.clear_access_channels(websocketpp::log::alevel::frame_header);

    //     c.init_asio();

    //     websocketpp::lib::error_code ec;
    //     con = c.get_connection(weburi, ec);
    //     con->add_subprotocol("janus-protocol");
    //     if(ec)
    //     {
    //         spdlog::warn("could not create connection because:{}", ec.message());
    //     }

    //     hdl = con->get_handle();
    //     c.connect(con);
    //     std::thread th(&client::run, &c);
    //     th.detach();
    // }
    // sleep(3);
    // c.send(hdl, "hello", websocketpp::frame::opcode::text);
    // c.close(hdl, websocketpp::close::status::normal, "");

    // m_endpoint.set_error_channels(websocketpp::log::elevel::all);
    // m_endpoint.set_access_channels(websocketpp::log::alevel::devel);
    // m_endpoint.clear_access_channels(websocketpp::log::alevel::frame_payload);
    // m_endpoint.init_asio();
    // m_endpoint.set_open_handler(std::bind(&on_open, std::placeholders::_1));
    // m_endpoint.listen(9002);
    // m_endpoint.start_accept();
    // m_endpoint.run();

    if(websocketOn)
    {
        std::thread serverTh = std::thread(startServerTh);
        serverTh.detach();
    }

}

jetsonEncoder::~jetsonEncoder()
{
    spdlog::warn("jetsonEncoder destructor ");
    // c.close(hdl, websocketpp::close::status::normal, "");
    if(ctx.enc)
        delete ctx.enc;
}

void jetsonEncoder::set_defaults(context_t * ctx)
{
    memset(ctx, 0, sizeof(context_t));

    ctx->bitrate = 2 * 1024* 1024;
    ctx->fps_n = 30;
    ctx->fps_d = 1;
    ctx->width = 1280;
    ctx->height = 720;
    ctx->out_file_path = new char[256];
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
        // spdlog::warn("i:{},plane.bytesused:{}", i, plane.bytesused);
    }
}

int jetsonEncoder::encodeFrame(uint8_t *yuv_bytes)
{
    spdlog::warn("jetsonEncoder::encodeFrame");
    int ret = 0;
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[MAX_PLANES];
    NvBuffer *buffer = ctx.enc->output_plane.getNthBuffer(frame_count);
    spdlog::warn("jetsonEncoder::529");
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
        spdlog::warn("jetsonEncoder::544");
        if (ctx.enc->output_plane.dqBuffer(v4l2_buf, &buffer, nullptr, 10) < 0) {
            cerr << "ERROR while DQing buffer at output plane" << endl;
            //abort(&ctx);
            //return cleanup(ctx, 1);
        }
    }

    spdlog::warn("jetsonEncoder::552");
    if (frame_count >= ctx.enc->output_plane.getNumBuffers()) {
        if (yuv_bytes)
            copyYuvToBuffer(yuv_bytes, *buffer);
        else
            v4l2_buf.m.planes[0].bytesused = 0;
    }
    spdlog::warn("ctx.enc->output_plane.qBuffer");
    ret = ctx.enc->output_plane.qBuffer(v4l2_buf, nullptr);
    if (ret < 0) {
        cerr << "Error while queueing buffer at output plane" << endl;
        //abort(&ctx);
        //return cleanup(ctx, 1);
    }

    frame_count++;
    return ret;
}

int jetsonEncoder::process(cv::Mat &img)
{
    spdlog::warn("jetsonEncoder::process");
    if(!webSocketSendStart)
        return RET_OK;

    cv::Mat yuvImg;
    cv::resize(img, img, cv::Size(1280,720));

    cvtColor(img, yuvImg,CV_BGR2YUV_I420);

    // spdlog::warn("yuvImg size:{}", yuvImg.total()*yuvImg.elemSize());

    encodeFrame(yuvImg.data); 
}

int jetsonEncoder::sendBase64(cv::Mat &img)
{
    if(!webSocketSendStart)
        return RET_OK;
    std::string data = Mat2Base64(img, "jpg");
    try
    {
        m_endpoint.send(g_hdl, data.c_str(), data.size(), websocketpp::frame::opcode::text);

    }
    catch(...)
    {
        spdlog::warn("websocket connection failed");
        webSocketSendStart = false;
        return RET_ERR;
    }

    return RET_OK;
}


