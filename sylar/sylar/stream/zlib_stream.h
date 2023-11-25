#pragma once
#ifndef __SYLAR_STREAMS_ZLIB_STREAM_H__
#define __SYLAR_STREAMS_ZLIB_STREAM_H__

#include "../stream.h"

#define ZLIB_WINAPI
#include <zlib.h>



namespace sylar {

	//处理数据压缩与解压缩流
	class ZlibStream :public Stream {
	public:
		using ptr = std::shared_ptr<ZlibStream>;


		enum Type {
			ZLIB,
			DEFLATE,
			GZIP
		};

		//压缩策略
		enum Strategy {
			DEFAULT = Z_DEFAULT_STRATEGY,
			FILTERED = Z_FILTERED,
			HUFFMAN = Z_HUFFMAN_ONLY,
			FIXED = Z_FIXED,
			RLE = Z_RLE
		};

		//压缩级别,压缩级别是一个0-9的数字，0压缩速度最快（压缩的过程），9压缩速度最慢，压缩率最大，0不压缩数
		enum CompressLevel {
			NO_COMPRESSION = Z_NO_COMPRESSION,
			BEST_SPEED = Z_BEST_SPEED,
			BEST_COMPRESSION = Z_BEST_COMPRESSION,
			DEFAULT_COMPRESSION = Z_DEFAULT_COMPRESSION
		};

		static ZlibStream::ptr CreateGzip(bool encode, uint32_t buff_size = 4096);
		static ZlibStream::ptr CreateZlib(bool encode, uint32_t buff_size = 4096);
		static ZlibStream::ptr CreateDeflate(bool encode, uint32_t buff_size = 4096);

		ZlibStream(bool encode, uint32_t buff_size = 4096);
		~ZlibStream();

		virtual int read(void* buffer, size_t length)override;
		virtual int read(ByteArray::ptr ba, size_t length)override;
		virtual int write(const void* buffer, size_t length)override;
		virtual int write(ByteArray::ptr ba, size_t length)override;
		virtual void close()override;

		//刷新
		int flush();

		//是否释放
		bool isFree()const { return m_free; }
		
		//设置是否释放内存
		void setFree(bool v) { m_encode = v; }

		//是否是压缩
		bool isEncode()const { return m_encode; }

		//设置压缩还是解压 true:false
		void setEncode(bool v) { m_encode = v; }

		std::vector<WSABUF>& getBuffers() { return m_buffs; }

		//获取结果字符串
		std::string getResult()const;

		//获取结果的序列化
		ByteArray::ptr getByteArray();

	private:

		static ZlibStream::ptr Create(bool encode, uint32_t buff_size = 4096,
			Type type = DEFLATE, int level = DEFAULT_COMPRESSION, int window_bits = 15
			, int memlevel = 8, Strategy strategy = DEFAULT);


		int init(Type type = DEFLATE, int level = DEFAULT_COMPRESSION, int window_bits = 15, int memlevel = 8, Strategy strategy = DEFAULT);

		int encode(const WSABUF* v, const uint64_t& size, bool finish);
		int decode(const WSABUF* v, const uint64_t& size, bool finish);

	private:

		z_stream m_zstream;
		//每WSABUF内存大小
		uint32_t m_buffSize;
		//是压缩还是解压
		bool m_encode;
		//结束后是否释放内存
		bool m_free;
		//存储结果
		std::vector<WSABUF> m_buffs;

	};

}

//typedef struct z_stream_s {
//
//	z_const Bytef* next_in;                   // 将要压缩数据的首地址
//
//	uInt               avail_in;                     // 将要压缩数据的长度
//
//	uLong            total_in;                     // 将要压缩数据缓冲区的长度
//
//	Bytef* next_out;                   // 压缩后数据保存位置。
//
//	uInt               avail_out;                    // 压缩后数据的长度
//
//	uLong            total_out;                    // 压缩后数据缓冲区的大小
//
//	z_const char* msg;                          // 存放最近的错误信息，NULL表示没有错误
//
//	struct internal_state FAR* state; /* not visible by applications */
//
//	alloc_func        zalloc;  /* used toallocate the internal state */
//
//	free_func        zfree;   /* used to free theinternal state */
//
//	voidpf             opaque;  /* private data object passed tozalloc and zfree */
//
//	int                 data_type;                   // 表示数据类型，文本或者二进制
//
//	uLong            adler;     /* adler32 value of the uncompressed data */
//
//	uLong            reserved;  /* reserved for future use */
//
//}  z_stream;

#endif