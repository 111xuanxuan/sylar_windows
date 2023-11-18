#include "zlib_stream.h"


namespace sylar {

	sylar::ZlibStream::ptr ZlibStream::CreateGzip(bool encode, uint32_t buff_size /*= 4096*/)
	{
		return Create(encode, buff_size, GZIP);
	}



	sylar::ZlibStream::ptr ZlibStream::CreateZlib(bool encode, uint32_t buff_size /*= 4096*/)
	{
		return Create(encode, buff_size, ZLIB);
	}

	sylar::ZlibStream::ptr ZlibStream::CreateDeflate(bool encode, uint32_t buff_size /*= 4096*/)
	{
		return Create(encode,buff_size,DEFLATE);
	}

	sylar::ZlibStream::ptr ZlibStream::Create(bool encode, uint32_t buff_size /*= 4096*/, Type type /*= DEFLATE*/, int level /*= DEFAULT_COMPRESSION*/, int window_bits /*= 15 */, int memlevel /*= 8*/, Strategy strategy /*= DEFAULT*/)
	{
		ZlibStream::ptr rt = std::make_shared<ZlibStream>(encode,buff_size);

		if (rt->init(type, level, window_bits, memlevel, strategy)==Z_OK) {
			return rt;
		}

		return nullptr;
	}

	ZlibStream::ZlibStream(bool encode, uint32_t buff_size /*= 4096*/)
		:m_buffSize(buff_size)
		,m_encode{encode}
		,m_free{true}
	{

	}

	ZlibStream::~ZlibStream()
	{
		if (m_free) {
			for (auto& i:m_buffs)
			{
				free(i.buf);
			}
		}

		if (m_encode) {
			//—πÀı
			deflateEnd(&m_zstream);
		}
		else
		{
			//Ω‚—π
			inflateEnd(&m_zstream);
		}

	}


	int ZlibStream::read(void* buffer, size_t length)
	{
		throw std::logic_error("ZlibStream::read is invalid");
	}

	int ZlibStream::read(ByteArray::ptr ba, size_t length)
	{
		throw std::logic_error("ZlibStream::read is invalid");
	}

	int ZlibStream::write(const void* buffer, size_t length)
	{
		WSABUF ivc;
		ivc.buf = (char*)buffer;
		ivc.len = length;
		if (m_encode) {
			return encode(&ivc, 1, false);
		}

	}

}