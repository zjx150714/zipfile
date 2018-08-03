#pragma once
#include <iostream>
#include <assert.h>
#include <string>
#include "HuffmanTree.h"
#include <algorithm>

typedef size_t LongType; //当文件很大时，可进行替换

struct CharInfo
{
	char _ch;        //字符
	LongType _count; //对应字符出现次数
	string _code; //编码

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
			//初始化
			_info[i]._ch = i;
			_info[i]._count = 0;
			_info[i]._code = "";
		}
	}

	//用于记录配置信息的类
	struct _HuffmanInfo
	{
		char _ch;	//字符
		LongType _count;	//对应字符出现个数
	};	

	//压缩
	void Compress(const char* filename)
	{
		assert(filename); //确保文件名不为空

		FILE* fout = fopen(filename, "rb");
		//assert(fout);

		//1、统计字符出现字数
		char ch = fgetc(fout);
		while (ch != EOF)
		{
			_info[(unsigned char)ch]._count++;
			ch = fgetc(fout);
		}

		//2、构建huffman树
		CharInfo invalid;
		invalid._count = 0;
		HaffmanTree<CharInfo> tree(_info, 256, invalid);

		//3、生成编码
		GenerateCode(tree.GetRoot());

		//4、压缩文件(创建一个压缩后的文件进行写入）
		string compressFile = filename;
		compressFile += ".huffman";
		FILE* fIn = fopen(compressFile.c_str(), "wb");
		assert(fIn);

		_HuffmanInfo info;
		size_t size = 0;
		//写配置信息，记录对应字符出现次数
		for (size_t i = 0; i < 256; ++i)
		{
			if (_info[i]._count > 0)
			{
				info._ch = _info[i]._ch;
				info._count = _info[i]._count;
				size = fwrite(&info, sizeof(_HuffmanInfo), 1, fIn);
				//fwrite返回写入对象的个数，确保写入成功
				assert(size == 1);
			}
		}

		//增添标记，方便解压缩
		info._count = 0;
		size = fwrite(&info, sizeof(_HuffmanInfo), 1, fIn);
		assert(size == 1);

		
		char value = 0;
		int count = 0;
		//重新读文件，将文件指针重新置为开始位置
		fseek(fout, 0, SEEK_SET);

		//向压缩文件中开始写入对应字符的huffman编码
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
			
				//满一个字节就写入
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

	//生成压缩编码
	void GenerateCode(Node* root) 
	{
		if (root == NULL)
			return;

		//只有叶子节点才是我们的字符，
		if (root->_left == NULL && root->_right == NULL)
		{
			//生成编码
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


	//解压缩
	void UnCopmpress(const char* filename)
	{
		assert(filename);

		//修改解压缩后的文件后缀
		string uncompress = filename;
		size_t pos = uncompress.rfind('.');
		assert(pos != string::npos);
		uncompress = uncompress.substr(0, pos);
		uncompress += ".uncompress";

		FILE* fin = fopen(uncompress.c_str(), "wb");
		assert(fin);

		FILE* fout = fopen(filename, "rb");
		assert(fout);
	
		//先读取配置信息，写入压缩文件中的CharInfo数组中，一会需要利用配置信息构造huffman树
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

		//重建huffman
		CharInfo invalid;
		invalid._count = 0;
		HaffmanTree<CharInfo> tree(_info, 256, invalid);
		Node* root = tree.GetRoot();
		//根节点的count就是读取的所有字符个数
		LongType charCount = root->_w._count;

		//解压缩
		char value = fgetc(fout);
		Node* cur = root;
		while (!feof(fout))
		{
			//对取出的一个字节分别探测
			for (int pos = 7; pos >= 0; --pos)
			{
				if (value & (1 << pos)) //1
					cur = cur->_right;
				else//0往左走
					cur = cur->_left;

				//判断叶子节点
				if (cur->_left == NULL && cur->_right == NULL)
				{
					fputc(cur->_w._ch, fin);//写入解压缩的文件
					cur = root;

					--charCount;
					if (charCount == 0)
						break;
				}
			}

			//该字符处理完成，进行下一个字符处理
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