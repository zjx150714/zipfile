#pragma once
#include <iostream>
#include <assert.h>
#include <string>
#include "HuffmanTree.h"
#include <algorithm>

typedef size_t LongType; //���ļ��ܴ�ʱ���ɽ����滻

struct CharInfo
{
	char _ch;        //�ַ�
	LongType _count; //��Ӧ�ַ����ִ���
	string _code; //����

	bool operator != (const CharInfo& info)const
	{
		return _count != info._count;
	}

	CharInfo operator+(const CharInfo info)
	{
		CharInfo ret;
		ret._count = _count + info._count;
		return ret;
	}

	bool operator< (const CharInfo& info)const
	{
		return _count < info._count;
	}
};

class FileCompress
{
	typedef HuffmanTreeNode<CharInfo> Node;

protected:
	CharInfo _info[256];

public:
	FileCompress()
	{
		for (size_t i = 0; i < 256; i++)
		{
			//��ʼ��
			_info[i]._ch = i;
			_info[i]._count = 0;
			_info[i]._code = "";
		}
	}

	//���ڼ�¼������Ϣ����
	struct _HuffmanInfo
	{
		char _ch;	//�ַ�
		LongType _count;	//��Ӧ�ַ����ָ���
	};	

	//ѹ��
	void Compress(const char* filename)
	{
		assert(filename); //ȷ���ļ�����Ϊ��

		FILE* fout = fopen(filename, "rb");
		//assert(fout);

		//1��ͳ���ַ���������
		char ch = fgetc(fout);
		while (ch != EOF)
		{
			_info[(unsigned char)ch]._count++;
			ch = fgetc(fout);
		}

		//2������huffman��
		CharInfo invalid;
		invalid._count = 0;
		HaffmanTree<CharInfo> tree(_info, 256, invalid);

		//3�����ɱ���
		GenerateCode(tree.GetRoot());

		//4��ѹ���ļ�(����һ��ѹ������ļ�����д�룩
		string compressFile = filename;
		compressFile += ".huffman";
		FILE* fIn = fopen(compressFile.c_str(), "wb");
		assert(fIn);

		_HuffmanInfo info;
		size_t size = 0;
		//д������Ϣ����¼��Ӧ�ַ����ִ���
		for (size_t i = 0; i < 256; ++i)
		{
			if (_info[i]._count > 0)
			{
				info._ch = _info[i]._ch;
				info._count = _info[i]._count;
				size = fwrite(&info, sizeof(_HuffmanInfo), 1, fIn);
				//fwrite����д�����ĸ�����ȷ��д��ɹ�
				assert(size == 1);
			}
		}

		//�����ǣ������ѹ��
		info._count = 0;
		size = fwrite(&info, sizeof(_HuffmanInfo), 1, fIn);
		assert(size == 1);

		
		char value = 0;
		int count = 0;
		//���¶��ļ������ļ�ָ��������Ϊ��ʼλ��
		fseek(fout, 0, SEEK_SET);

		//��ѹ���ļ��п�ʼд���Ӧ�ַ���huffman����
		ch = fgetc(fout);
		while (!feof(fout))
		{
			string& code = _info[(unsigned char)ch]._code;
			for (size_t i = 0; i < code.size(); ++i)
			{
				value <<= 1;
				if (code[i] == '1')
					value |= 1;
				else
					value |= 0;
				++count;
			
				//��һ���ֽھ�д��
				if (count == 8)
				{
					fputc(value, fIn);
					value = 0;
					count = 0;
				}
			}

			ch = fgetc(fout);
		}

		if (count != 0)
		{
			value <<= (8 - count);
			fputc(value,fIn);
		}

		fclose(fIn);
		fclose(fout);
	}

	//����ѹ������
	void GenerateCode(Node* root) 
	{
		if (root == NULL)
			return;

		//ֻ��Ҷ�ӽڵ�������ǵ��ַ���
		if (root->_left == NULL && root->_right == NULL)
		{
			//���ɱ���
			string& code = _info[(unsigned char)root->_w._ch]._code;
			Node* cur = root;
			Node* parent = cur->_parent;
			while (parent)
			{
				if (parent->_left == cur)
					code.push_back('0');
				else
					code.push_back('1');

				cur = parent;
				parent = cur->_parent;
			}

			reverse(code.begin(), code.end());
			//_info[root->_w._ch]._code = code;
			return;
		}
		GenerateCode(root->_left);
		GenerateCode(root->_right);
	}


	//��ѹ��
	void UnCopmpress(const char* filename)
	{
		assert(filename);

		//�޸Ľ�ѹ������ļ���׺
		string uncompress = filename;
		size_t pos = uncompress.rfind('.');
		assert(pos != string::npos);
		uncompress = uncompress.substr(0, pos);
		uncompress += ".uncompress";

		FILE* fin = fopen(uncompress.c_str(), "wb");
		assert(fin);

		FILE* fout = fopen(filename, "rb");
		assert(fout);
	
		//�ȶ�ȡ������Ϣ��д��ѹ���ļ��е�CharInfo�����У�һ����Ҫ����������Ϣ����huffman��
		while (1)
		{
			_HuffmanInfo info;
			size_t size = fread(&info, sizeof(_HuffmanInfo), 1, fout);
			assert(size == 1);

			if (info._count)
			{
				_info[(unsigned char)info._ch]._ch = info._ch;
				_info[(unsigned char)info._ch]._count = info._count;
			}
			else
				break;
		}

		//�ؽ�huffman
		CharInfo invalid;
		invalid._count = 0;
		HaffmanTree<CharInfo> tree(_info, 256, invalid);
		Node* root = tree.GetRoot();
		//���ڵ��count���Ƕ�ȡ�������ַ�����
		LongType charCount = root->_w._count;

		//��ѹ��
		char value = fgetc(fout);
		Node* cur = root;
		while (!feof(fout))
		{
			//��ȡ����һ���ֽڷֱ�̽��
			for (int pos = 7; pos >= 0; --pos)
			{
				if (value & (1 << pos)) //1
					cur = cur->_right;
				else//0������
					cur = cur->_left;

				//�ж�Ҷ�ӽڵ�
				if (cur->_left == NULL && cur->_right == NULL)
				{
					fputc(cur->_w._ch, fin);//д���ѹ�����ļ�
					cur = root;

					--charCount;
					if (charCount == 0)
						break;
				}
			}

			//���ַ�������ɣ�������һ���ַ�����
			value = fgetc(fout);
		}

		fclose(fin);
		fclose(fout);
	}
};

void TestFileCompress()
{
	FileCompress fc;
	fc.Compress("Input.txt");
	fc.UnCopmpress("Input.txt.huffman");
}