#include "ofxNDISender.h"

ofxNDISender::ofxNDISender(std::string name){
	const NDIlib_send_create_t descriptor = {name.c_str(), NULL, TRUE, FALSE};
	_sender = NDIlib_send_create(&descriptor);
	if (!NDIlib_initialize())
	{
		ofLogFatalError("ofxNDISender") << "cannot run NDI";
		ofLogNotice("ofxNDI") << "Most likely because the CPU is not sufficient (see SDK documentation). You can check this directly with a call to NDIlib_is_supported_CPU()";
	}

	unsigned int width = 640;
	unsigned int height = 360;

	_frame = {
			// Resolution
			(int)width,
			(int)height,
			// We will stick with RGB color space. Note however that it is generally better to
			// use YCbCr colors spaces if you can since they get end-to-end better video quality
			// and better performance because there is no color dconversion
			NDIlib_FourCC_type_BGRA,
			// The frame-eate
			30000, 1001,
			// The aspect ratio (16:9)
			(float)(width) /
					height,
			// This is not a progressive frame
			FALSE,
			// Timecode (synthesized for us !)
			NDIlib_send_timecode_synthesize,
			// The video memory used for this frame
			(uint8_t *)malloc(width * height * 4),
			// The line to line stride of this image
			(int)(width)*4};
	memset((void *)_frame.p_data, 0, width * height * 4);
}

void ofxNDISender::setMetaData(std::string longName,
															 std::string shortName,
															 std::string manufacturer,
															 std::string version,
															 std::string session,
															 std::string modelName,
															 std::string serial)
{

	NDIlib_metadata_frame_t metaData;

	std::string xml = "<ndi_product long_name=\"" + longName + "\" " +
										"short_name=\"" + shortName + "\" " +
										"manufacturer=\"" + manufacturer + "\" " +
										"version=\"" + version + "\" " +
										"session=\"" + session + "\" " +
										"model_name=\"" + modelName + "\" " +
										"serial=\"" + serial + "\">";

	std::vector<char> chars(xml.c_str(), xml.c_str() + xml.size() + 1u);

	metaData.p_data = &chars[0];
	metaData.timecode = NDIlib_send_timecode_synthesize;
	NDIlib_send_add_connection_metadata(_sender, &metaData);
}

void ofxNDISender::send(ofPixels & pixels){
	if (_frame.xres != pixels.getWidth() || _frame.yres != pixels.getHeight())
	{
		_frame = {
				// Resolution
				(int)(pixels.getWidth()),
				(int)(pixels.getHeight()),
				// We will stick with RGB color space. Note however that it is generally better to
				// use YCbCr colors spaces if you can since they get end-to-end better video quality
				// and better performance because there is no color dconversion
				NDIlib_FourCC_type_BGRA,
				// The frame-eate
				30000, 1001,
				// The aspect ratio (16:9)
				(float)(pixels.getWidth()) /
						(float)(pixels.getHeight()),
				// This is not a progressive frame
				FALSE,
				// Timecode (synthesized for us !)
				NDIlib_send_timecode_synthesize,
				// The video memory used for this frame
				(uint8_t *)malloc(pixels.getWidth() * pixels.getHeight() * 4),
				// The line to line stride of this image
				(int)(pixels.getWidth()) * 4};
	}

	switch (pixels.getPixelFormat())
	{
	case OF_PIXELS_RGB:
	case OF_PIXELS_BGR:
	{
		int index = 0;
		for (auto line : pixels.getLines())
		{
			for (auto pixel : line.getPixels())
			{
				_frame.p_data[index] = pixel[2];
				_frame.p_data[index + 1] = pixel[1];
				_frame.p_data[index + 2] = pixel[0];
				_frame.p_data[index + 3] = 255;
				index += 4;
			}
		}
		break;
	}
	case OF_PIXELS_RGBA:
	case OF_PIXELS_BGRA:
	{
		int index = 0;
		for (auto line : pixels.getLines())
		{
			for (auto pixel : line.getPixels())
			{
				_frame.p_data[index] = pixel[2];
				_frame.p_data[index + 1] = pixel[1];
				_frame.p_data[index + 2] = pixel[0];
				_frame.p_data[index + 3] = pixel[3];
				index += 4;
			}
		}
	}
	default:
	{
		ofLogError("ofxNDISender") << "pixel type " << ofToString(pixels.getPixelFormat()) << " is not supported yet";
		return;
		break;
	}
	}

	NDIlib_send_send_video_v2(_sender, &_frame);
}
