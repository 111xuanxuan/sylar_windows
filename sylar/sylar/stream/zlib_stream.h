#pragma once
#ifndef __SYLAR_STREAMS_ZLIB_STREAM_H__
#define __SYLAR_STREAMS_ZLIB_STREAM_H__

#include "../stream.h"

#define ZLIB_WINAPI
#include <zlib.h>



namespace sylar {

	//��������ѹ�����ѹ����
	class ZlibStream :public Stream {
	public:
		using ptr = std::shared_ptr<ZlibStream>;


		enum Type {
			ZLIB,
			DEFLATE,
			GZIP
		};

		//ѹ������
		enum Strategy {
			DEFAULT = Z_DEFAULT_STRATEGY,
			FILTERED = Z_FILTERED,
			HUFFMAN = Z_HUFFMAN_ONLY,
			FIXED = Z_FIXED,
			RLE = Z_RLE
		};

		//ѹ������,ѹ��������һ��0-9�����֣�0ѹ���ٶ���죨ѹ���Ĺ��̣���9ѹ���ٶ�������ѹ�������0��ѹ����
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

		//ˢ��
		int flush();

		//�Ƿ��ͷ�
		bool isFree()const { return m_free; }
		
		//�����Ƿ��ͷ��ڴ�
		void setFree(bool v) { m_encode = v; }

		//�Ƿ���ѹ��
		bool isEncode()const { return m_encode; }

		//����ѹ�����ǽ�ѹ true:false
		void setEncode(bool v) { m_encode = v; }

		std::vector<WSABUF>& getBuffers() { return m_buffs; }

		//��ȡ����ַ���
		std::string getResult()const;

		//��ȡ��������л�
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
		//ÿWSABUF�ڴ��С
		uint32_t m_buffSize;
		//��ѹ�����ǽ�ѹ
		bool m_encode;
		//�������Ƿ��ͷ��ڴ�
		bool m_free;
		//�洢���
		std::vector<WSABUF> m_buffs;

	};

}

//typedef struct z_stream_s {
//
//	z_const Bytef* next_in;                   // ��Ҫѹ�����ݵ��׵�ַ
//
//	uInt               avail_in;                     // ��Ҫѹ�����ݵĳ���
//
//	uLong            total_in;                     // ��Ҫѹ�����ݻ������ĳ���
//
//	Bytef* next_out;                   // ѹ�������ݱ���λ�á�
//
//	uInt               avail_out;                    // ѹ�������ݵĳ���
//
//	uLong            total_out;                    // ѹ�������ݻ������Ĵ�С
//
//	z_const char* msg;                          // �������Ĵ�����Ϣ��NULL��ʾû�д���
//
//	struct internal_state FAR* state; /* not visible by applications */
//
//	alloc_func        zalloc;  /* used toallocate the internal state */
//
//	free_func        zfree;   /* used to free theinternal state */
//
//	voidpf             opaque;  /* private data object passed tozalloc and zfree */
//
//	int                 data_type;                   // ��ʾ�������ͣ��ı����߶�����
//
//	uLong            adler;     /* adler32 value of the uncompressed data */
//
//	uLong            reserved;  /* reserved for future use */
//
//}  z_stream;

#endif