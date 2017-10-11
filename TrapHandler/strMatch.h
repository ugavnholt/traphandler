#ifndef _STRMATCH_HEAD
#define _STRMATCH_HEAD

#include "stdafx.h"


//#include <wchar.h>
#include <vector>
//#include <list>

#pragma warning (disable: 4996)

///////////////////////////////////////////////
//
// Module to use ov string match rules to match a string
//
// The CVar class is used to manage the variables defined
// by the match expressions, as well as user-defined variables
//
// the CStrMatch class is used to manage the expressions themself

using namespace std;

struct SVarStruct
{
	wchar_t *varName;
	wchar_t *varValue;
};

enum exprTypes 
{ 
	undefined = 0,	
	strSeq = 1,		// abc or <!(abc|def|xyz).var>
	charSeq = 2,	// <!n[abc].var>
	asteric = 3,	// <n*.var>
	word = 4,		// <!n@.var>
	numSeq = 5,		// <!n#.var>
	ip = 6,			// <!ip{10.10-11.*.*}>
	oid = 7			// <!oid{.1.3.6.1.4.1.*}>
};

// First define a structure, that we use to hold
// the parsed expressions given in expr, either in constructor
// or in setExpre(whcar_t *expr)
struct exprStruct
{
	exprTypes type;
	wchar_t *pStart;	// first matched char
	wchar_t *pEnd;		// last matched char
	vector<wchar_t *>token;		// the string contained in <...>, except !, and n
	bool negate;		// the exclamtion mark in <!...>
	bool iCase;			// whether icase is enabled for this expression, controlled with <i>...</i> flags
	wchar_t *varName;	// Name of the variable is any
	size_t n;			// the number in <n...>
};

class CVar
{
private:
	vector<SVarStruct*> varList;
	vector<SVarStruct*>::iterator it;	// Iterator used to point to the current value in the list
	size_t nCount;

public:
	
	CVar() :
	nCount(0),
	it(varList.begin())
	{
	}

	~CVar()
	{
		while(!varList.empty())
		{
			delete [] varList.back()->varName;
			delete [] varList.back()->varValue;
			delete varList.back();
			varList.pop_back();
			nCount--;
		}
	}

	inline void MoveFirst() { it = varList.begin(); }
	inline bool MoveNext() 
	{ 
		if (it != varList.end())
		{
			it++;
			return true;
		}
		return false;
	}

	inline wchar_t *GetName()  { if(it != varList.end()) { return (*it)->varName; } else return NULL; }
	inline wchar_t *GetValue() { if(it != varList.end()) { return (*it)->varValue; } else return NULL; }
	inline size_t GetCount() { return nCount; }

	inline bool EOL() {	if(it != varList.end())	return false; else return true;	}
	void SetVar(wchar_t *name, wchar_t *value);
	wchar_t *GetValueByName(wchar_t *name);
};

class CStrMatch
{
private:
	CVar *vars;
	int lastError;
	int direction;
	vector<exprStruct*> Expressions;	// the tokinized list of expressions
	size_t nExpressions;
	wchar_t *errorMsg;
	bool freeExpressions();
	bool anchorLeft;
	bool anchorRight;
	bool quoted;	// flag used by parseQuote to inform caller if the char is quoted or not
	bool quote;
	bool icase;
	inline const wchar_t parseQuote(wchar_t ch)
	{
		lastError = 0;
		if(!quote && ch == L'\\')
		{
			quote = true;
			quoted = false;
			return L'\0';
			//wprintf(L"found quote char\n");
		}
		else if(quote && ch == L't')
		{
			quote = false;
			quoted = false;
			return L'\t';
		}
		else if(quote && ch == L'n')
		{
			quote = false;
			quoted = false;
			return L'\n';
		}
		else if(quote && ch == L'\\')
		{
			quote = false;
			quoted = true;
			return ch;
		}
		else if(quote && ch == L'r')
		{
			quote = false;
			quoted = false;
			return L'\r';
		}
		else if(quote && ch == L'$')
		{
			quote = false;
			quoted = true;
			return ch;
		}
		else if(quote && ch == L'[')
		{
			quote = false;
			quoted = true;
			return ch;
		}
		else if(quote && ch == L']')
		{
			quote = false;
			quoted = true;
			return ch;
		}
		else if(quote && ch == L'{')
		{
			quote = false;
			quoted = true;
			return ch;
		}
		else if(quote && ch == L'(')
		{
			quote = false;
			quoted = true;
			return ch;
		}
		else if(quote && ch == L')')
		{
			quote = false;
			quoted = true;
			return ch;
		}
		else if(quote && ch == L'|')
		{
			quote = false;
			quoted = true;
			return ch;
		}
		else if(quote && ch == L'*')
		{
			quote = false;
			quoted = true;
			return ch;
		}
		else if(quote && ch == L'@')
		{
			quote = false;
			quoted = true;
			return ch;
		}
		else if(quote && ch == L'#')
		{
			quote = false;
			quoted = true;
			return ch;
		}
		else if(quote && ch == L'.')
		{
			quote = false;
			quoted = true;
			return ch;
		}
		else if(quote && ch == L'<')
		{
			quote = false;
			quoted = true;
			return ch;
		}
		else if(quote && ch == L'>')
		{
			quote = false;
			quoted = true;
			return ch;
		}
		else if(quote)
		{
			lastError = -4;
			return L'\0';
		}
		else
		{
			quote = false;
			quoted = false;
			return ch;
		}
	}


	//////////////////////////////////////////////
	//
	// This function takes an expression, and build the exprVector
	// used to evaluate strings
	//
	bool tokenize(const wchar_t *Expression);
	const wchar_t *parseTag(const wchar_t *startPos);

	// each of the following functions return the pointer to the next char after a successfull match
	// for the given expression type, or NULL if no match for the given char pos

	wchar_t *getNextMatchPos_charSeq(size_t exprIndex, wchar_t *startPos, wchar_t *endPos, bool rooted);
	wchar_t *getNextMatchPos_strSeq(size_t exprIndex, wchar_t *startPos, wchar_t *endPos, bool rooted);
	wchar_t *getNextMatchPos_asteric(size_t exprIndex, wchar_t *startPos, wchar_t *endPos, bool rooted);
	wchar_t *getNextMatchPos_word(size_t exprIndex, wchar_t *startPos, wchar_t *endPos, bool rooted);
	wchar_t *getNextMatchPos_numSeq(size_t exprIndex, wchar_t *startPos, wchar_t *endPos, bool rooted);
	wchar_t *getNextMatchPos_ip(size_t exprIndex, wchar_t *startPos, wchar_t *endPos, bool rooted);
	wchar_t *getNextMatchPos_oid(size_t exprIndex, wchar_t *startPos, wchar_t *endPos, bool rooted);

	inline bool alphaChar(wchar_t c)
	{
		if((unsigned short)c >= 0x41 && c <= 0x7a) // Basic latin, upper and lower range
			return true;
		else if((unsigned short)c >= 0xc0 && c <= 0xfe) // Latin-1 upper and lower
			return true;
		else if((unsigned short)c >= 0x101 && (unsigned short)c <= 0x17e) // Latin Extended A
			return true;
		else if((unsigned short)c >= 0x180 && (unsigned short)c <= 0x24f) // Latin Extended B
			return true;
		else 
			return false;
	}
	inline wchar_t upChar(wchar_t c)
	{
		if((unsigned short)c >= 0x61 && c <= 0x7a) // Basic latin, upper version 0x20 less than lower
			return (c - (unsigned short)0x20);
		else if((unsigned short)c >= 0xe0 && c <= 0xfe) // Latin-1
			return (c - (unsigned short)0x20);
		else if((unsigned short)c >= 0x101 && (unsigned short)c <= 0x17e && (unsigned short)c % 2 == 1) // Latin Extended A
			return (c - (unsigned short)1);
		else if((unsigned short)c >= 0x180 && (unsigned short)c <= 0x24f && (unsigned short)c % 2 == 0) // Latin Extended B
			return (c - (unsigned short)1);
		else 
			return c;
	}

public:
	CStrMatch();
	CStrMatch(wchar_t *Expression);
	inline bool SetExpr(const wchar_t *expr) { return tokenize(expr);}
	wchar_t *GetErrorMessage();
	int GetLastError() { return lastError; }
	/////////////////////////////
	//
	// MatchString is the actual method used to compare the expression to a given string
	// if there is a match 0 is returned, if there isn't a match 1 is returned, if there was a
	// negative number is returned witch is the lastError value
	int MatchString(const wchar_t *str);	// Matches the defined expression against a string
	inline CVar *GetVars() { return vars; }
	~CStrMatch();
};

#endif