#include "zlib_stream.h"
#include "../macro.h"


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

		//初始化
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
			//压缩结束
			deflateEnd(&m_zstream);
		}
		else
		{
			//解压结束
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
		}else{
			return decode(&ivc, 1, false);
		}

	}

	int ZlibStream::write(ByteArray::ptr ba, size_t length)
	{
		std::vector<WSABUF> buffers;
		ba->getReadBuffers(buffers, length);

		if (m_encode) {
			return encode(&buffers[0], buffers.size(), false);
		}
		else {
			return decode(&buffers[0], buffers.size(), false);
		}

	}

	void ZlibStream::close()
	{
		//刷新结果
		flush();
	}



	int ZlibStream::flush()
	{
		WSABUF ivc;
		ivc.buf = nullptr;
		ivc.len = 0;

		//是压缩
		if (m_encode) {
			return encode(&ivc, 1, true);
		}//解压
		else {
			return decode(&ivc, 1, true);
		}
	}

	std::string ZlibStream::getResult() const
	{
		std::string rt;
		for (auto& i : m_buffs) {
			rt.append(i.buf, i.len);
		}
		return rt;
	}

	sylar::ByteArray::ptr ZlibStream::getByteArray()
	{
		ByteArray::ptr ba = std::make_shared<ByteArray>();
		for (auto& i:m_buffs)
		{
			ba->write(i.buf, i.len);
		}
		ba->setPosition(0);
		return ba;
	}

	int ZlibStream::init(Type type /*= DEFLATE*/, int level /*= DEFAULT_COMPRESSION*/, int window_bits /*= 15*/, int memlevel /*= 8*/, Strategy strategy /*= DEFAULT*/)
	{
		SYLAR_ASSERT((level >= 0 && level <= 9) || level == DEFAULT_COMPRESSION);
		SYLAR_ASSERT((window_bits >= 8 && window_bits <= 15));
		SYLAR_ASSERT((memlevel >= 1 && memlevel <= 9));

		memset(&m_zstream, 0, sizeof(m_zstream));

		m_zstream.zalloc = Z_NULL;
		m_zstream.zfree = Z_NULL;
		m_zstream.zfree = Z_NULL;

		switch (type)
		{
		case DEFLATE:
			window_bits = -window_bits;
			break;
		case GZIP:
			window_bits += 16;
			break;
		case ZLIB:
		default:
			break;
		}

		if (m_encode) {
			//压缩
			return deflateInit2(&m_zstream, level, Z_DEFLATED, window_bits, memlevel, strategy);
		}
		else {
			//解压缩
			return inflateInit2(&m_zstream, window_bits);
		}


	}

	int ZlibStream::encode(const WSABUF* v, const uint64_t& size, bool finish)
	{
		int ret = 0;
		int flush = 0;
		for (uint64_t i=0;i<size;++i)
		{
			//压缩的数据长度
			m_zstream.avail_in = v[i].len;
			//压缩数据的首地址
			m_zstream.next_in = (Bytef*)v[i].buf;

			flush = finish ? (i == size - 1 ? Z_FINISH : Z_NO_FLUSH) : Z_NO_FLUSH;

			WSABUF* ivc = nullptr;

			do 
			{
				//如果非空且最后一个WSABUF的长度不等于m_buffSize
				if (!m_buffs.empty()&&m_buffs.back().len!=m_buffSize) {
					ivc = &m_buffs.back();
				}
				else
				{
					WSABUF vc;
					vc.buf = (char*)malloc(m_buffSize);
					vc.len = 0;
					m_buffs.push_back(vc);
					ivc = &m_buffs.back();
				}

				m_zstream.avail_out = m_buffSize - ivc->len;
				m_zstream.next_out = (Bytef*)ivc->buf + ivc->len;

				//压缩
				ret = deflate(&m_zstream, flush);

				//压缩出错
				if (ret == Z_STREAM_ERROR) {
					return ret;
				}

				ivc->len = m_buffSize - m_zstream.avail_out;



			} while (m_zstream.avail_out==0);//压缩后的数据长度为0

		}

		if (flush == Z_FINISH) {
			//压缩结束
			deflateEnd(&m_zstream);
		}
		return Z_OK;

	}

	int ZlibStream::decode(const WSABUF* v, const uint64_t& size, bool finish)
	{
		int ret = 0;
		int flush = 0;
		for (uint64_t i = 0; i < size; ++i)
		{
			//压缩的数据长度
			m_zstream.avail_in = v[i].len;
			//压缩数据的首地址
			m_zstream.next_in = (Bytef*)v[i].buf;

			flush = finish ? (i == size - 1 ? Z_FINISH : Z_NO_FLUSH) : Z_NO_FLUSH;

			WSABUF* ivc = nullptr;

			do
			{
				//如果非空且最后一个WSABUF的长度不等于m_buffSize
				if (!m_buffs.empty() && m_buffs.back().len != m_buffSize) {
					ivc = &m_buffs.back();
				}
				else
				{
					WSABUF vc;
					vc.buf = (char*)malloc(m_buffSize);
					vc.len = 0;
					m_buffs.push_back(vc);
					ivc = &m_buffs.back();
				}

				m_zstream.avail_out = m_buffSize - ivc->len;
				m_zstream.next_out = (Bytef*)ivc->buf + ivc->len;

				//解压
				ret = inflate(&m_zstream, flush);

				//解压出错
				if (ret == Z_STREAM_ERROR) {
					return ret;
				}

				ivc->len = m_buffSize - m_zstream.avail_out;



			} while (m_zstream.avail_out == 0);//压缩后的数据长度为0

		}

		if (flush == Z_FINISH) {
			//解压结束
			inflateEnd(&m_zstream);
		}
		return Z_OK;
	}

}