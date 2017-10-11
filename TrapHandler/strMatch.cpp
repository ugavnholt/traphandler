#include "stdafx.h"
//#ifndef _STRMATCH_CODE
//#define _STRMATCH_CODE

//#include <wchar.h>
//#include <vector>
//#include <list>
#include "strMatch.h"
//#include <windows.h>

using namespace std;

#pragma warning (disable :4244)
#pragma warning (disable :4018)

#define freeExpr() if(expr != NULL) \
					{\
						while(!expr->token.empty())\
						{\
							delete [] expr->token.back();\
							expr->token.pop_back();\
						}\
						delete expr;\
					}

wchar_t *CStrMatch::getNextMatchPos_ip(size_t exprIndex, wchar_t *startPos, wchar_t *endPos, bool rooted)
{
	// logic is the same as oid, but check for number ranges, and number of '.'s

	wchar_t *pStr = startPos;
	for(vector<wchar_t*>::iterator i=Expressions.at(exprIndex)->token.begin(); i != Expressions.at(exprIndex)->token.end(); i++)
	{
		//wprintf(L"Token: %s\n", (*i));
	}
	return NULL;
}

wchar_t *CStrMatch::getNextMatchPos_oid(size_t exprIndex, wchar_t *startPos, wchar_t *endPos, bool rooted)
{
	wchar_t *pStr = startPos;
	vector<wchar_t*>::iterator i = Expressions.at(exprIndex)->token.begin();

	bool ip = false;	// flag to determine if we are evaluating an ip expression, or an oid
	int num = -1;
//	unsigned int value;
	bool match = false;

	while(pStr != endPos)
	{
		if(*pStr == L'0' || *pStr == L'1' || *pStr == L'2' || *pStr == L'3' || *pStr == L'4' || *pStr == L'5' || *pStr == L'6' ||
			*pStr == L'7' || *pStr == L'8' || *pStr == L'9' || *pStr == L'.')
		{
			match = false;
			// the initial . doesn't matter, in an oid its optional, in front of a ip address, it simply isn't part of the address
			// unless we are rooted.
			if(*pStr == L'.' && ip && rooted)
			{
				//wprintf(L"leading '.' found in an ip address\n");
				return NULL;
			}
			else if(*pStr == L'.' && ip)
			{
				pStr++;
				if(*pStr == L'0' || *pStr == L'1' || *pStr == L'2' || *pStr == L'3' || *pStr == L'4' || 
					*pStr == L'5' || *pStr == L'6' || *pStr == L'7' || *pStr == L'8' || *pStr == L'9')
				{
					if(direction > 0)
						Expressions.at(exprIndex)->pStart = pStr;
					else
						Expressions.at(exprIndex)->pEnd = pStr;
				}
				else
				{
					//wprintf(L"no number following the initial dot in ip address\n");
					break;
				}

				// from here we have a number...
			}
			else if(*pStr == L'.')
			{
				if(direction > 0)
					Expressions.at(exprIndex)->pStart = pStr;
				else
					Expressions.at(exprIndex)->pEnd = pStr;
				pStr++;
			}

		}
		if(!rooted)
			pStr++;
		else
			return NULL;
	}

	return NULL;
}

//////////////////////////////////////////////
// Match n words, if n == 0, match one word only
//
// this function breaks when 
wchar_t *CStrMatch::getNextMatchPos_word(size_t exprIndex, wchar_t *startPos, wchar_t *endPos, bool rooted)
{
	wchar_t *pStr = startPos;
	size_t sepChars = 0;

	if(direction > 0)
		Expressions.at(exprIndex)->pStart = pStr;
	else
		Expressions.at(exprIndex)->pEnd = pStr+1;

	// the number of words we have to match
	size_t n = Expressions.at(exprIndex)->n;
	if(n==0)
		n++;

	//wprintf(L"Searching with negflag: %i, for %i separators\n", Expressions.at(exprIndex)->negate, n);
	if(n > abs(endPos-startPos))
		return NULL;

	while(pStr != endPos)
	{
		//wprintf(L"Processing char: %c\n", *pStr);
		// we need to count the number of no-matches
		// if negated we count the number of alpha chars
		// otherwise we count the number of separator chars
		if((!alphaChar(*pStr) && !Expressions.at(exprIndex)->negate) || ( alphaChar(*pStr) && Expressions.at(exprIndex)->negate ))
		{
			sepChars++;
			//wprintf(L"Found sep char# %i\n", sepChars);
		}
		else if(sepChars == 0)
		{	// we did not have a separator char, so lets set our start position here
			if(direction > 0)
				Expressions.at(exprIndex)->pEnd = pStr;
			else
				Expressions.at(exprIndex)->pStart = pStr+1;
		}

		// the rooted flag tells us wether we have to go to the next char, or break the loop
		
		if(rooted && sepChars == 0)
		{
			//wprintf(L"rooted, and first n chars did not match a word\n");
			return NULL;
		}

		// if we found the number of sepchars, break;
		if(sepChars == n)
			break;
		pStr+=direction;
	}	// main searchstring loop

	// we treat end of line as a separator character
	if(pStr == endPos)
		sepChars++;

	// we found what we searched for, now evaluate the result
	if(sepChars == n)
	{
		if(direction > 0)
			Expressions.at(exprIndex)->pEnd = pStr;
		else
			Expressions.at(exprIndex)->pStart = pStr;

		//wprintf(L"startPos: %c, endPos: %c\n", *Expressions.at(exprIndex)->pStart, *Expressions.at(exprIndex)->pEnd);
		// if we have another expression in line, launch it
		if( exprIndex+direction <= nExpressions-1 && pStr-direction != endPos && exprIndex+direction >= 0)
		{
			pStr+=direction;
			if(Expressions.at(exprIndex+direction)->type == strSeq) return getNextMatchPos_strSeq(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == charSeq) return getNextMatchPos_charSeq(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == asteric) return getNextMatchPos_asteric(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == word) return getNextMatchPos_word(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == numSeq) return getNextMatchPos_numSeq(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == ip) return getNextMatchPos_ip(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == oid) return getNextMatchPos_oid(exprIndex+direction, pStr, endPos, true);
		}
		else if(pStr == endPos)
		{	// if we processed the last char in the string
			return pStr;
		}
		else if(anchorLeft && anchorRight)	// we have more chars in string
			return NULL;
		else
			return pStr;
	}

	// at this case we reached the end of the search string
	// but did not find enough separator chars
	return NULL;
}

wchar_t *CStrMatch::getNextMatchPos_numSeq(size_t exprIndex, wchar_t *startPos, wchar_t *endPos, bool rooted)
{
	// excactly like calling charSeq with [0123456789]

	// check if we have to match fewer chars than the string length
	if(Expressions.at(exprIndex)->n > 0 && abs(endPos-startPos)+1 < Expressions.at(exprIndex)->n)
		return NULL;
	// we only have one token
	wchar_t *charList = L"0123456789";
	wchar_t *cStr = charList;
	bool match = false;
	size_t mChars = 0;

	wchar_t *pStr = startPos;

	while(pStr != endPos+direction)
	{
		match = false;

		// check if any of the chars in out match string matches
		while(*cStr != L'\0')
		{
			if( (!Expressions.at(exprIndex)->iCase && *cStr == *pStr) || (Expressions.at(exprIndex)->iCase && upChar(*cStr) == upChar(*pStr)))
			{
				match = true;
				if(!Expressions.at(exprIndex)->negate)
					mChars++;
				break;
			}
			cStr++;
		}
		if(!match && Expressions.at(exprIndex)->negate)
			mChars++;
		
		// reverse match flag, if negate flag is set
		if(match && Expressions.at(exprIndex)->negate)
			match = false;
		else if(!match && Expressions.at(exprIndex)->negate)
			match = true;

		// reset our expression pointer
		cStr = charList;

		// break if the first char didn't match, and we are rooted
		if((mChars== 0 && !match && rooted) )
			break;

		if( mChars > 0 && !match )
		{
			pStr-=direction;
			break;
		}

		if(mChars==1 && match)
		{
			if(direction > 0)
			{
				Expressions.at(exprIndex)->pStart = pStr;
			}
			else 
				Expressions.at(exprIndex)->pEnd = pStr+1;
		}

		if( match && mChars == Expressions.at(exprIndex)->n )
		{
			break;
		}
		
		pStr+=direction;
	}

	// mChars is the number of characters matched...  || pStr == endPos+direction
	if(mChars > 0 && (Expressions.at(exprIndex)->n == mChars || Expressions.at(exprIndex)->n == 0 || Expressions.at(exprIndex)->n == 0 ) )
	{
		if(direction > 0)
			Expressions.at(exprIndex)->pEnd = pStr+1;
		else 
			Expressions.at(exprIndex)->pStart = pStr;

		if( exprIndex+direction <= nExpressions-1 && pStr-direction != endPos && exprIndex+direction >= 0)
		{
			pStr+=direction;
			if(Expressions.at(exprIndex+direction)->type == strSeq) return getNextMatchPos_strSeq(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == charSeq) return getNextMatchPos_charSeq(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == asteric) return getNextMatchPos_asteric(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == word) return getNextMatchPos_word(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == numSeq) return getNextMatchPos_numSeq(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == ip) return getNextMatchPos_ip(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == oid) return getNextMatchPos_oid(exprIndex+direction, pStr, endPos, true);
		}
		else if(pStr == endPos)
		{
			return pStr;
		}
		else if(anchorLeft && anchorRight && pStr != endPos && (exprIndex+direction > nExpressions-1 || nExpressions-1 < 0)) 
		{
			return NULL;
		}
		return pStr;
	}

	return NULL;
}

wchar_t *CStrMatch::getNextMatchPos_charSeq(size_t exprIndex, wchar_t *startPos, wchar_t *endPos, bool rooted)
{
	// check if we have to match fewer chars than the string length
	if(Expressions.at(exprIndex)->n > 0 && abs(endPos-startPos)+1 < Expressions.at(exprIndex)->n)
		return NULL;
	// we only have one token
	wchar_t *cStr = Expressions.at(exprIndex)->token.front();
	bool match = false;
	size_t mChars = 0;

	wchar_t *pStr = startPos;

	while(pStr != endPos+direction)
	{
		match = false;

		// check if any of the chars in out match string matches
		while(*cStr != L'\0')
		{
			if( (!Expressions.at(exprIndex)->iCase && *cStr == *pStr) || (Expressions.at(exprIndex)->iCase && upChar(*cStr) == upChar(*pStr)))
			{
			
				match = true;
				if(!Expressions.at(exprIndex)->negate)
					mChars++;
				break;
			}
			cStr++;
		}
		if(!match && Expressions.at(exprIndex)->negate)
			mChars++;
		
		// reverse match flag, if negate flag is set
		if(match && Expressions.at(exprIndex)->negate)
			match = false;
		else if(!match && Expressions.at(exprIndex)->negate)
			match = true;

		// reset our expression pointer
		cStr = Expressions.at(exprIndex)->token.front();

		// break if the first char didn't match, and we are rooted
		if((mChars== 0 && !match && rooted) )
			break;

		if( mChars > 0 && !match )
		{
			pStr-=direction;
			break;
		}

		if(mChars==1 && match)
		{
			if(direction > 0)
			{
				Expressions.at(exprIndex)->pStart = pStr;
			}
			else 
				Expressions.at(exprIndex)->pEnd = pStr+1;
		}

		if( match && mChars == Expressions.at(exprIndex)->n )
		{
			break;
		}
		
		pStr+=direction;
	}

	// mChars is the number of characters matched...  || pStr == endPos+direction
	if(mChars > 0 && (Expressions.at(exprIndex)->n == mChars || Expressions.at(exprIndex)->n == 0 || Expressions.at(exprIndex)->n == 0 ) )
	{
		if(direction > 0)
			Expressions.at(exprIndex)->pEnd = pStr+1;
		else 
			Expressions.at(exprIndex)->pStart = pStr;

		if( exprIndex+direction <= nExpressions-1 && pStr-direction != endPos && exprIndex+direction >= 0)
		{
			pStr+=direction;
			if(Expressions.at(exprIndex+direction)->type == strSeq) return getNextMatchPos_strSeq(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == charSeq) return getNextMatchPos_charSeq(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == asteric) return getNextMatchPos_asteric(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == word) return getNextMatchPos_word(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == numSeq) return getNextMatchPos_numSeq(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == ip) return getNextMatchPos_ip(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == oid) return getNextMatchPos_oid(exprIndex+direction, pStr, endPos, true);
		}
		else if(pStr == endPos)
		{
			return pStr;
		}
		else if(anchorLeft && anchorRight && pStr != endPos && (exprIndex+direction > nExpressions-1 || nExpressions-1 < 0)) 
		{
			return NULL;
		}
		return pStr;
	}

	return NULL;
}

wchar_t *CStrMatch::getNextMatchPos_asteric(size_t exprIndex, wchar_t *startPos, wchar_t *endPos, bool rooted)
{
	// check for early exit, do we have enough characters?
	if(Expressions.at(exprIndex)->n > abs(endPos-startPos)+1)
	{
		return NULL;
	}

	// wprintf(L"exprIndex : %u, direction: %i, nExpressions: %u\n", exprIndex, direction, nExpressions);
	// double anchored, but with too many chars?
	if(Expressions.at(exprIndex)->n > 0)
	{
		if(anchorLeft && anchorRight && Expressions.at(exprIndex)->n != abs(endPos-startPos)+1)
		{
			if( exprIndex+direction > nExpressions-1 || exprIndex+direction < 0)
			{
				return NULL;
			}
		}

		// if n > 0 and both left and right anchored, just copy chars
		if(anchorLeft && anchorRight)
		{
			Expressions.at(exprIndex)->pStart = startPos;
			Expressions.at(exprIndex)->pEnd = startPos+Expressions.at(exprIndex)->n;
		}

		// if left anchored and n>0 just copy chars
		else if(direction > 0)
		{
			Expressions.at(exprIndex)->pStart = startPos;
			Expressions.at(exprIndex)->pEnd = startPos + Expressions.at(exprIndex)->n+1;
		}
		else if(direction < 0)	// right anchored
		{
			Expressions.at(exprIndex)->pStart = startPos - (Expressions.at(exprIndex)->n-1);
			Expressions.at(exprIndex)->pEnd = startPos+1;
		}
		
		wchar_t *pStr = startPos + ((Expressions.at(exprIndex)->n) * direction);

		if( exprIndex+direction <= nExpressions-1 && pStr-direction != endPos && exprIndex+direction >= 0)
		{
			if(Expressions.at(exprIndex+direction)->type == strSeq) return getNextMatchPos_strSeq(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == charSeq) return getNextMatchPos_charSeq(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == asteric) return getNextMatchPos_asteric(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == word) return getNextMatchPos_word(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == numSeq) return getNextMatchPos_numSeq(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == ip) return getNextMatchPos_ip(exprIndex+direction, pStr, endPos, true);
			else if(Expressions.at(exprIndex+direction)->type == oid) return getNextMatchPos_oid(exprIndex+direction, pStr, endPos, true);
		}
		else return pStr;
	}	// if n > 0

	// if we are the last expression, and only need to match n chars, check for double rooted expression
	// and determine true or false return
	// given the early checks, we know we have the excact number of characters, if n>0
	
	
	else if( exprIndex+direction > nExpressions-1 || exprIndex+direction < 0)
	{
		// wprintf(L"####\nTEST\n####\n");
		if(direction > 0)
		{
			Expressions.at(exprIndex)->pStart = startPos;
			Expressions.at(exprIndex)->pEnd = endPos+1;
		}
		else
		{
			Expressions.at(exprIndex)->pEnd = startPos;
			Expressions.at(exprIndex)->pStart = endPos;
		}
		return Expressions.at(exprIndex)->pEnd;
	}
	
	// skip n chars, since that is the minimum we need to return true
	wchar_t *pStr = startPos+(Expressions.at(exprIndex)->n*direction);

	// At this point we have no option, but to try the next expression on startPos in non-rooted mode
	// it will return its startposition, which will be out end- or start position+1 depending on the direction
	wchar_t *retPos;

	//wprintf(L"Starting next parser, endpos Char: %c\n", *endPos);
	if(Expressions.at(exprIndex+direction)->type == strSeq) 
		retPos = getNextMatchPos_strSeq(exprIndex+direction, pStr, endPos, false);
	else if(Expressions.at(exprIndex+direction)->type == charSeq) 
		retPos = getNextMatchPos_charSeq(exprIndex+direction, pStr, endPos, false);
	else if(Expressions.at(exprIndex+direction)->type == asteric) 
		retPos = getNextMatchPos_asteric(exprIndex+direction, pStr, endPos, false);
	else if(Expressions.at(exprIndex+direction)->type == word) 
		retPos = getNextMatchPos_word(exprIndex+direction, pStr, endPos, false);
	else if(Expressions.at(exprIndex+direction)->type == numSeq) 
		retPos = getNextMatchPos_numSeq(exprIndex+direction, pStr, endPos, false);
	else if(Expressions.at(exprIndex+direction)->type == ip) 
		retPos = getNextMatchPos_ip(exprIndex+direction, pStr, endPos, false);
	else if(Expressions.at(exprIndex+direction)->type == oid) 
		retPos = getNextMatchPos_oid(exprIndex+direction, pStr, endPos, false);

	if(retPos == NULL)
		return NULL;

	retPos = Expressions.at(exprIndex+direction)->pStart;

	if(direction > 0)
	{
		Expressions.at(exprIndex)->pStart = startPos;
		Expressions.at(exprIndex)->pEnd = retPos;
	}
	else
	{
		Expressions.at(exprIndex)->pEnd = retPos;
		Expressions.at(exprIndex)->pStart = endPos;
	}

	//wprintf(L"retPos: %i:%c\n", (int)retPos, *(retPos));
	//wprintf(L"pStart: %i, pEnd: %i\n", Expressions.at(exprIndex)->pStart, Expressions.at(exprIndex)->pEnd);

	return retPos;
}


///////////////////////////////////////////////////////////////////////////////////////
// TODO: Rewrite to take negateFlag into consideration
// main evaluation need to use match flag, instead of taking evaluation action in-scope

wchar_t *CStrMatch::getNextMatchPos_strSeq(size_t exprIndex, wchar_t *startPos, wchar_t *endPos, bool rooted)
{
	wchar_t *pStr;

	wchar_t *cStr;
	wchar_t *cEndPos;

	for(vector<wchar_t *>::iterator st = Expressions.at(exprIndex)->token.begin(); st != Expressions.at(exprIndex)->token.end(); st++)
	{
		cStr = (*st);
		pStr = startPos;
		cEndPos = (*st)+wcslen((*st));

		// if right to left direction, move patPtr to end of string
		if(direction < 0)
		{
			cEndPos = (*st)-1;
			while(*cStr != L'\0')
				cStr++;
			cStr--;
		}
 
		while(pStr != endPos+direction && (abs(cEndPos-cStr) <= abs(endPos-pStr)+1))
		{
			if((!Expressions.at(exprIndex)->iCase && *pStr == *cStr) ||
				( Expressions.at(exprIndex)->iCase && upChar(*pStr) == upChar(*cStr)))
			{
				if(direction > 0)
					Expressions.at(exprIndex)->pStart = pStr;
				else
					Expressions.at(exprIndex)->pEnd = pStr+1;
				cStr+=direction;
				pStr+=direction;

				while( ((!Expressions.at(exprIndex)->iCase && *pStr == *cStr) || (Expressions.at(exprIndex)->iCase && upChar(*pStr) == upChar(*cStr))) && pStr != endPos+direction && cStr != cEndPos+direction) 
				{
					cStr+=direction;
					pStr+=direction;
				}	// compare each other char in pat string
				
				/////////
				// determine if we had a full match
				
				if(cStr == cEndPos)
				{
					if(direction > 0)
						Expressions.at(exprIndex)->pEnd = pStr;
					else
						Expressions.at(exprIndex)->pStart = pStr+1;
					
					if( exprIndex+direction <= nExpressions-1 && pStr-direction != endPos && exprIndex+direction >= 0)
					{
						if(Expressions.at(exprIndex+direction)->type == strSeq) return getNextMatchPos_strSeq(exprIndex+direction, pStr, endPos, true);
						else if(Expressions.at(exprIndex+direction)->type == charSeq) return getNextMatchPos_charSeq(exprIndex+direction, pStr, endPos, true);
						else if(Expressions.at(exprIndex+direction)->type == asteric) { return getNextMatchPos_asteric(exprIndex+direction, pStr, endPos, true); }
						else if(Expressions.at(exprIndex+direction)->type == word) return getNextMatchPos_word(exprIndex+direction, pStr, endPos, true);
						else if(Expressions.at(exprIndex+direction)->type == numSeq) return getNextMatchPos_numSeq(exprIndex+direction, pStr, endPos, true);
						else if(Expressions.at(exprIndex+direction)->type == ip) return getNextMatchPos_ip(exprIndex+direction, pStr, endPos, true);
						else if(Expressions.at(exprIndex+direction)->type == oid) return getNextMatchPos_oid(exprIndex+direction, pStr, endPos, true);
					}
					else if(pStr-direction != endPos && anchorLeft && anchorRight) // we have more chars, but no more expressions, and are double anchored
					{
						return NULL;
					}
					else
					{
						return pStr;	// return SUCCESS - we are the last pattern
						
					}
				} // we had a full match
				
				// reset searchstring
				cStr = (*st);
				if(direction < 0)
				{
					while(*cStr != L'\0')
						cStr++;
					cStr--;
				}
				
			} // if first char matches
			else if(!rooted)
			{
				pStr+=direction;
			}
			else
			{
				break;	// try next pattern if any
			}

		} // parse each char in search string
	}	// parse each pattern to search for

	// wprintf(L"matchcheck\n");
	return NULL;
}

int CStrMatch::MatchString(const wchar_t *str)
{
	
	if(Expressions.empty())
	{
		lastError = -21;
		return lastError;
	}
	if(nExpressions == 1 && Expressions.at(0)->type == asteric)
	{
		if(Expressions.at(0)->varName == NULL)
			return 0;	// we had a match

		Expressions.at(0)->pStart = (wchar_t*)str;
		Expressions.at(0)->pEnd = (wchar_t*)str+wcslen(str);
		goto setVars;
	}
	
	// if we only have a stringtoken, with a single token
	if((anchorLeft || anchorRight) && nExpressions == 1 && Expressions.at(0)->type == strSeq && Expressions.at(0)->token.size() == 1)
	{
		int result = 0;
		size_t len;
		
		if(anchorRight && anchorLeft)
		{
			len = wcslen(Expressions.at(0)->token.front());
			if(Expressions.at(0)->iCase)
				result = wcsicmp(Expressions.at(0)->token.front(), str);
			else
				result = wcscmp(Expressions.at(0)->token.front(), str);

			if(result != 0)
				return 1;	// no match

			Expressions.at(0)->pStart = (wchar_t*)str;
			Expressions.at(0)->pEnd = (wchar_t*)str+len;
			goto setVars;
		}
		else if(anchorLeft)
		{
			len = wcslen(Expressions.at(0)->token.front());
			if(Expressions.at(0)->iCase)
				result = wcsnicmp(Expressions.at(0)->token.front(), str, len);
			else
				result = wcsncmp(Expressions.at(0)->token.front(), str, len);

			if(result != 0)
				return 1;	// no match

			Expressions.at(0)->pStart = (wchar_t*)str;
			Expressions.at(0)->pEnd = (wchar_t*)str+len;
			goto setVars;
		}
		else if(anchorRight)
		{
			len = wcslen(Expressions.at(0)->token.front());
			size_t strLen = wcslen(str);
			if(len > strLen)
				return 1;	// strings can't match, since the pattern length, is greater than string length
			wchar_t *strStart = (wchar_t*)str+(strLen-len);

			if(Expressions.at(0)->iCase)
				result = wcsicmp(Expressions.at(0)->token.front(), strStart);
			else
				result = wcscmp(Expressions.at(0)->token.front(), strStart);
			
			if(result != 0)
				return 1;	// no match

			Expressions.at(0)->pStart = strStart;
			Expressions.at(0)->pEnd = (wchar_t*)str+strLen;
			goto setVars;
		}
	}	// only one string match

	size_t nStartExpr;
	wchar_t *startPtr;
	wchar_t *endPtr;
	bool rooted = false;
	size_t myLen = 0;
	if(anchorLeft || anchorRight)
		rooted = true;

	// if right anchored we need to start at the end of the string
	if(anchorRight && !anchorLeft)
	{
		myLen = wcslen(str)-1;
		startPtr = (wchar_t*)str+myLen;
		endPtr = (wchar_t*)str;
		nStartExpr = nExpressions-1;
		direction = -1;
		
	}
	else
	{
		myLen = wcslen(str)-1;
		startPtr = (wchar_t*)str;
		endPtr = (wchar_t*)str+myLen;
		direction = 1;
		nStartExpr = 0;
	}
	
	if(Expressions.at(nStartExpr)->type == strSeq)
	{
		if(getNextMatchPos_strSeq(nStartExpr, startPtr, endPtr, rooted) != NULL)
			goto setVars; // Match
		else if(lastError == 0)
			return 1; // no match
		else
			return lastError;
	}
	else if(Expressions.at(nStartExpr)->type == charSeq)
	{
		if(getNextMatchPos_charSeq(nStartExpr, startPtr, endPtr, rooted) != NULL)
			goto setVars; // Match
		else if(lastError == 0)
			return 1; // no match
		else
			return lastError;
	}
	else if(Expressions.at(nStartExpr)->type == asteric)
	{
		if(getNextMatchPos_asteric(nStartExpr, startPtr, endPtr, rooted) != NULL)
			goto setVars; // Match
		else if(lastError == 0)
			return 1; // no match
		else
			return lastError;
	}
	else if(Expressions.at(nStartExpr)->type == word)
	{
		if(getNextMatchPos_word(nStartExpr, startPtr, endPtr, rooted) != NULL)
			goto setVars; // Match
		else if(lastError == 0)
			return 1; // no match
		else
			return lastError;
	}
	else if(Expressions.at(nStartExpr)->type == numSeq)
	{
		if(getNextMatchPos_numSeq(nStartExpr, startPtr, endPtr, rooted) != NULL)
			goto setVars; // Match
		else if(lastError == 0)
			return 1; // no match
		else
			return lastError;
	}
	else if(Expressions.at(nStartExpr)->type == ip)
	{
		if(getNextMatchPos_ip(nStartExpr, startPtr, endPtr, rooted) != NULL)
			goto setVars; // Match
		else if(lastError == 0)
			return 1; // no match
		else
			return lastError;
	}
	else if(Expressions.at(nStartExpr)->type == oid)
	{
		if(getNextMatchPos_oid(nStartExpr, startPtr, endPtr, rooted) != NULL)
			goto setVars; // Match
		else if(lastError == 0)
			return 1; // no match
		else
			return lastError;
	}

setVars:
	//wprintf(L"\nVariables defined: \n");
	if(vars == NULL)
		vars = new CVar();
	wchar_t *tmpVal;
	wchar_t *tmpName;
	size_t namelen;
	size_t vallen;
	for(size_t i=0; i<nExpressions; i++)
	{
		if(Expressions.at(i)->varName != NULL && Expressions.at(i)->pEnd != NULL && Expressions.at(i)->pStart != NULL)
		{
			vallen = Expressions.at(i)->pEnd-Expressions.at(i)->pStart;
			tmpVal = new wchar_t[vallen+1];
			wcsncpy(tmpVal, Expressions.at(i)->pStart, vallen);
			tmpVal[vallen] = L'\0';

			namelen = wcslen(Expressions.at(i)->varName);
			tmpName = new wchar_t[namelen+1];
			wcscpy(tmpName, Expressions.at(i)->varName);

			vars->SetVar(tmpName, tmpVal);
			//wprintf(L"\t%s: '%s'\n", tmpName, tmpVal);
		}
	}
	return 0; // match
}

CStrMatch::CStrMatch() :
	lastError(0),
	nExpressions(0),
	errorMsg(NULL),
	vars(NULL)
{
}

CStrMatch::CStrMatch(wchar_t *Expression)
{
	CStrMatch();
	tokenize(Expression);
}

CStrMatch::~CStrMatch()
{
	if(errorMsg != NULL)
		delete [] errorMsg;
	freeExpressions();
	if(vars != NULL)
		delete vars;
}

bool CStrMatch::tokenize(const wchar_t *Expression)
{
	if(Expression == NULL || *Expression == L'\0')
	{
		lastError = -2;	// no expression set
		return false;
	}

	freeExpressions();

	anchorLeft = false;
	anchorRight = false;
	quote = false;
	
	// initialize some working variables
	const wchar_t	*pStr = Expression;
	size_t	strLen = wcslen(pStr);
	
	wchar_t *tmpStr = new wchar_t[strLen+1];
	wchar_t *ptStr = tmpStr;
	wchar_t ch;
	quote = false;
	size_t	nVal		= 0;
	bool	negateFlag  = false;
	icase		= false;
	exprTypes type		= undefined;
	size_t len = 0;

	// step 1 - determine if we are left anchored
	if(*pStr == L'^')
	{
		anchorLeft = true;
		if(*(pStr+1) == L'<' && *(pStr+2) == L'*')
			anchorLeft = false;
		pStr++;
	}
		
	// main parsing loop
	while(pStr <= Expression+strLen)
	{
		ch = parseQuote(*pStr);
		if(lastError != 0)
			goto Cleanup;
		if(ch == L'\0') // if we had an unqouted \\ char, parse the next ones
		{
			if(pStr == Expression+strLen-1) // is it the last char in string?
			{
				lastError = -9; // invalid string, eos is quoted
				goto Cleanup;
			}
			pStr++;
			ch = parseQuote(*pStr);
			if(lastError != 0)
				goto Cleanup;
		}
		
		if((ch != L'\0' && ch != L'<') || (quoted && ch == L'<'))
		{
			if(pStr == Expression+strLen-1 && !quoted  && ch == L'$')
			{
				//wprintf(L"### pStr: %u:%u - %c\n", pStr, Expression+strLen-1, ch);
			}
			else
			{
				*ptStr = ch;
				ptStr++;
			}
		}	
		if((!quoted && ch == L'<') || (pStr == Expression+strLen-1))
		{
			// if we have a string before tag, create an expression of it
			//wprintf(L"%u:%u, %i\n", tmpStr, ptStr, anchorRight);
			if(pStr == Expression+strLen-1 && *pStr == L'$' && !quoted)
				anchorRight = true;
			
			len = ptStr-tmpStr;

			if(tmpStr != ptStr)
			{	// set a new expression of string type
				//wprintf(L"tmpStr, before \\0 appended: %s\n", tmpStr);
				exprStruct *expr = new exprStruct();
				expr->iCase = icase;
				expr->n = 0;
				expr->negate = false;
				expr->pEnd = NULL;
				expr->pStart = NULL;
				expr->varName = NULL;
				expr->type = strSeq;
				
				wchar_t *tokenStr = new wchar_t[ptStr-tmpStr+1];
				expr->token.push_back(tokenStr);

				size_t nChars = 0;
				ptStr = tmpStr;
				wchar_t ch2;
				while(ptStr <= tmpStr+len)
				{
					ch2 = parseQuote(*ptStr);
					if(lastError != 0)
						goto Cleanup;
					if (ch2 != L'\0')
					{
						tokenStr[nChars] = ch2;
						nChars++;
					}
					ptStr++;
				}
				
				tokenStr[nChars-1] = L'\0';
				
				Expressions.push_back(expr);
				nExpressions++;

				ptStr = tmpStr;
			}

			// start the initial tag parsing, unless end of string
			if(pStr != Expression+strLen-1)	
				pStr = parseTag(pStr);
			
			if(pStr == NULL)
				goto Cleanup;
		}

		pStr++;
	}
	
	if(anchorRight && Expressions.at(nExpressions-1)->type == asteric && Expressions.at(nExpressions-1)->n == 0)
		anchorRight = false;

	// save the last bit of text if any as an expression
	/*
	wprintf(L"nExpressions: %i\n", nExpressions);
	wprintf(L"Parse results for string: %s\n", Expression);
	
	if(anchorLeft && !anchorRight)
		wprintf(L"String is left anchored\n");
	else if(anchorRight && !anchorLeft)
		wprintf(L"String is right anchored\n");
	else if(anchorRight && anchorLeft)
		wprintf(L"String is anchored in both ends\n");
	else
		wprintf(L"String is not anchored\n");
	for(size_t i=0; i < nExpressions; i++)
	{
		wprintf(L"\n------------------------------------------------\n");
		wprintf(L"Expression #:\t%i\n", i);
		wprintf(L"ICase flag:\t%i\n", Expressions.at(i)->iCase);
		wprintf(L"n counter:\t%i\n", Expressions.at(i)->n);
		wprintf(L"NegateFlag:\t%i\n", Expressions.at(i)->negate);
		wprintf(L"Type int:\t%i\n", Expressions.at(i)->type);
		wprintf(L"Var Name:\t%s\n", Expressions.at(i)->varName);
		for(vector<wchar_t*>::iterator j=Expressions.at(i)->token.begin(); j!=Expressions.at(i)->token.end(); j++)
			wprintf(L"\tToken:\t\t%s\n", (*j));
	}
	*/
	

Cleanup:
	if(tmpStr != NULL)
		delete [] tmpStr;
	if (lastError != 0)
		return false;
	else
		return true;
}

const wchar_t *CStrMatch::parseTag(const wchar_t *startPos)
{
	size_t	nVal		= 0;
	bool	negateFlag  = false;
	exprTypes type		= undefined;
	size_t	strLen		= wcslen(startPos);
	const wchar_t *pStr = startPos;
	exprStruct *expr = NULL;
	wchar_t *varStr = NULL;

	pStr++;

	wchar_t ch = parseQuote(*pStr);
	if(lastError != 0)
		return NULL;
	if(ch == L'\0')
	{
		lastError = -8;
		return NULL;
	}
	// check if first char is the negate char
	if(ch == L'!')
	{
		negateFlag = true;
		pStr++;
		ch = parseQuote(*pStr);
		if(lastError != 0)
			return NULL;
	}
	
	// check for a number following the negate char
	if(ch >= L'0' && ch <= L'9')
	{
		nVal = _wtoi(pStr);
		if(nVal <=0)
		{
			lastError = -5;
			return NULL;
		}
		while(*pStr >= L'0' && *pStr <= L'9')
			pStr++;
		ch = parseQuote(*pStr);
		if(lastError != 0)
			return NULL;
	}
	
	if((ch == L'i' || ch == L'I') && *(pStr+1) == L'>')
	{
		if(nVal != 0)
		{
			lastError = -6;
			return NULL;
		}
		if(negateFlag)
		{
			lastError = -7;
			return NULL;
		}
		icase = true;
		if(*(pStr+1) != L'>')
		{
			lastError = -3;
			return NULL;
		}
		else return pStr+1;
	}
	else if(ch == L'/' && (*(pStr+1) == L'i' || *(pStr+1) == L'I') && *(pStr+2) == L'>')
	{
		if(nVal != 0)
		{
			lastError = -6;
			return NULL;
		}
		if(negateFlag)
		{
			lastError = -7;
			return NULL;
		}
		icase = false;
		if(*(pStr+2) != L'>')
		{
			lastError = -3;
			return NULL;
		}
		else return pStr+2;
	}
	//////////////////////////////////////////
	//
	// Begin searching for tags
	//
	///////////////////////////
	// char list
	else if(ch == L'[')							// char sequence <!n[abc].var>
	{
		type = charSeq;

		pStr++;
		ch = parseQuote(*pStr);
		if(lastError != 0)
			return NULL;
		if(ch == L'\0')
		{
			pStr++;
			ch = parseQuote(*pStr);
			if(lastError != 0)
				return NULL;
		}

		if(ch == L']')
		{
			lastError = -13;	// char sequence, but no chars
			return NULL;
		}

		wchar_t *tokenStr = new wchar_t[wcslen(pStr)];
		wchar_t *tokPtr = tokenStr;

		expr = new exprStruct();	// we need to create the exprStruct, to build the token list

		// parse the token, that should be enclosed in [] tags
		while((ch != L']' || quoted && ch == L']') && pStr <=startPos+strLen)
		{
			*tokPtr++ = ch;
			pStr++;
			ch = parseQuote(*pStr);
			if(lastError != 0)
			{
				freeExpr();
				return NULL;
			}
			if(ch == L'\0')
			{
				pStr++;
				ch = parseQuote(*pStr);
				if(lastError != 0)
				{
					freeExpr();
					return NULL;
				}
			}
			
		}
		*tokPtr = L'\0';
		expr->token.push_back(tokenStr);
	}
	///////////////////////////
	// Asteric
	else if(ch == L'*')							// asteric <n*.var>
	{
		if(negateFlag)
		{
			lastError = -10;	// negate flag not supported for <*> expressions
			return NULL;
		}
		type = asteric;
	}
	////////////////////////////
	// Stringlist
	else if(ch == L'(')							// string list <!(abc|def).var>
	{
		type = strSeq;

		pStr++;
		ch = parseQuote(*pStr);
		if(lastError != 0)
			return NULL;
		if(ch == L'\0')
		{
			pStr++;
			ch = parseQuote(*pStr);
			if(lastError != 0)
				return NULL;
		}

		if(ch == L')')
		{
			lastError = -13;	// char sequence, but no chars
			return NULL;
		}

		wchar_t *tokenStr = new wchar_t[wcslen(pStr)];
		wchar_t *tokPtr = tokenStr;

		expr = new exprStruct();	// we need to create the exprStruct, to build the token list

		// parse the token, that should be enclosed in () tags
		while(pStr <= startPos+strLen)
		{
			if(ch == L'>' && !quoted)
				break;
			if((ch == L'|' || ch == L')') && !quoted)	// separator char between strings
			{
				if(tokPtr == tokenStr)
				{
					freeExpr();
					delete [] tokenStr;
					lastError = -15;	// Empty string in string sequence expression
					return NULL;
				}
				//tokPtr--;	// we don't want the separator char
				*(tokPtr) = L'\0';
				expr->token.push_back(tokenStr);

				tokenStr = new wchar_t[wcslen(pStr)];
				tokPtr = tokenStr;
				if(ch == L')' && !quoted)
					break;
				// pStr++;
				ch = parseQuote(*pStr);
				if(lastError != 0)
				{
					freeExpr();
					delete [] tokenStr;
					return NULL;
				}
			}
			else if(ch != L'\0')
				*tokPtr++ = ch;
			pStr++;
			ch = parseQuote(*pStr);
			if(lastError != 0)
			{
				freeExpr();
				delete [] tokenStr;
				return NULL;
			}			
		}
		// pStr++;
		delete [] tokenStr;
	}
	///////////////////////////
	// number sequence
	else if(ch == L'#')							// number sequence <!n#.var>
		type = numSeq;
	///////////////////////////
	// IP Address
	else if((ch == L'I' || ch == L'i') && (*(pStr+1) == L'P' || *(pStr+1) == L'p') && *(pStr+2) == L'{')
	{											// ip address <!ip{10.10.10-11.*}.var>
		type = ip;

		pStr += 3;
		ch = parseQuote(*pStr);
		if(lastError != 0)
			return NULL;
		if(ch == L'\0')
		{
			lastError = -17;
			return NULL;
		}

		if(ch == L'}')
		{
			lastError = -16;	// char sequence, but no chars
			return NULL;
		}

		wchar_t *tokenStr = new wchar_t[wcslen(pStr)];
		wchar_t *tokPtr = tokenStr;

		expr = new exprStruct();	// we need to create the exprStruct, to build the token list

		// parse the token, that should be enclosed in () tags
		while(pStr <= startPos+strLen)
		{
			if(ch == L'>' && !quoted)
				break;
			if((ch == L'}') && !quoted)	// separator char between strings
			{
				*(tokPtr) = L'\0';

				/////////////////////////////////////
				// Verify that the oid expression is correct
				// and push each element into the token vector
				tokPtr = tokenStr;
				size_t len = 0;
				while(*tokPtr != L'\0')
				{
					if(*tokPtr == L'\\')
					{
						lastError = -22;		// quoting is not allowed within an ip or oid expression
						delete [] tokenStr;
						return NULL;
					}
					if(*tokPtr == L'.' || *(tokPtr+1) == L'\0')
					{
						// we have parsed our first element
						if(*(tokPtr+1) == L'\0')
						{
							len++;
							tokPtr++;
						}
						wchar_t *element = new wchar_t[len+1];
						wcsncpy(element, tokPtr-len, len);
						element[len] = L'\0';
						//wprintf(L"found element: '%s'\n", element);
						expr->token.push_back(element);
						len = 0;
						if(*tokPtr == L'\0')
							break;
						tokPtr++;
					}
					else if(*tokPtr != L'0' && *tokPtr != L'1' && *tokPtr != L'2' && *tokPtr != L'3' && *tokPtr != L'4' && *tokPtr != L'5' && 
						*tokPtr != L'6' && *tokPtr != L'7' && *tokPtr != L'8' && *tokPtr != L'9' && *tokPtr != L'*' && *tokPtr != L'-' && *tokPtr != L',')
					{
						//wprintf(L"char: %c\n", *tokPtr);
						lastError = -23;		// invalid character found in oid or ip expression, only '0-9','-','*',',' and '.' allowed
						delete [] tokenStr;
						return NULL;
					}
					else
					{
						len++;
						tokPtr++;
					}
				}

				// expr->token.push_back(tokenStr);
				delete [] tokenStr;
				tokenStr = NULL;

				break;
			}
			*tokPtr++ = ch;
			pStr++;
			ch = parseQuote(*pStr);
			if(lastError != 0)
			{
				freeExpr();
				delete [] tokenStr;
				return NULL;
			}			
		}
		if(tokenStr != NULL)
			delete [] tokenStr;
	}
	///////////////////////////
	// oid
	else if((ch == L'o' || ch == L'O') && (*(pStr+1) == L'i' || *(pStr+1) == L'I') && (*(pStr+2) == L'd' || *(pStr+2) == L'D') && *(pStr+3) == L'{')
	{											// oid <!oid{.1.3.6.1.4.1.2-3.*}.var>
		type = oid;

		pStr += 4;
		ch = parseQuote(*pStr);
		if(lastError != 0)
			return NULL;
		if(ch == L'\0')
		{
			lastError = -19;
			return NULL;
		}

		if(ch == L'}')
		{
			lastError = -20;	// char sequence, but no chars
			return NULL;
		}

		wchar_t *tokenStr = new wchar_t[wcslen(pStr)];
		wchar_t *tokPtr = tokenStr;

		expr = new exprStruct();	// we need to create the exprStruct, to build the token list

		// parse the token, that should be enclosed in () tags
		while(pStr <= startPos+strLen)
		{
			if(ch == L'>' && !quoted)
				break;
			if((ch == L'}') && !quoted)	// end of oid spec
			{
				*(tokPtr) = L'\0';

				/////////////////////////////////////
				// Verify that the oid expression is correct
				// and push each element into the token vector
				tokPtr = tokenStr;
				size_t len = 0;
				while(*tokPtr != L'\0')
				{
					if(*tokPtr == L'\\')
					{
						lastError = -22;		// quoting is not allowed within an ip or oid expression
						delete [] tokenStr;
						return NULL;
					}
					if(*tokPtr == L'.' || *(tokPtr+1) == L'\0')
					{
						// we have parsed our first element
						if(*(tokPtr+1) == L'\0')
						{
							len++;
							tokPtr++;
						}
						wchar_t *element = new wchar_t[len+1];
						wcsncpy(element, tokPtr-len, len);
						element[len] = L'\0';
						//wprintf(L"found element: '%s'\n", element);
						expr->token.push_back(element);
						len = 0;
						if(*tokPtr == L'\0')
							break;
						tokPtr++;
					}
					else if(*tokPtr != L'0' && *tokPtr != L'1' && *tokPtr != L'2' && *tokPtr != L'3' && *tokPtr != L'4' && *tokPtr != L'5' && 
						*tokPtr != L'6' && *tokPtr != L'7' && *tokPtr != L'8' && *tokPtr != L'9' && *tokPtr != L'*' && *tokPtr != L'-' && *tokPtr != L',')
					{
						//wprintf(L"char: %c\n", *tokPtr);
						lastError = -23;		// invalid character found in oid or ip expression, only '0-9','-','*',',' and '.' allowed
						delete [] tokenStr;
						return NULL;
					}
					else
					{
						len++;
						tokPtr++;
					}
				}

				// expr->token.push_back(tokenStr);
				delete [] tokenStr;
				tokenStr = NULL;
				break;
			}
			*tokPtr++ = ch;
			pStr++;
			ch = parseQuote(*pStr);
			if(lastError != 0)
			{
				freeExpr();
				delete [] tokenStr;
				return NULL;
			}			
		}
		if(tokenStr != NULL)
			delete [] tokenStr;
	}
	///////////////////////////
	// Word sequence
	else if(ch == L'@')							// word sequece <!n@.var>
		type = word;
	else
	{
		lastError = -18;
		return NULL;
	}

	////////////////////////////////////////////////
	//
	// parse the variable string if its there, and push the expression

	pStr++;
	ch = parseQuote(*pStr);
	if(lastError != 0)
	{
		freeExpr();
		return NULL;
	}
	if(ch == L'\0')
	{
		freeExpr();
		lastError = -8;
		return NULL;
	}
	if(ch == L'.')		// we have a variable name
	{
		pStr++;
		ch = parseQuote(*pStr);
		if(lastError != 0)
		{
			freeExpr();
			return NULL;
		}
		if(ch == L'\0')
		{
			freeExpr();
			lastError = -8;
			return NULL;
		}

		varStr = new wchar_t[wcslen(pStr)]; // don't add one for \0 - we have . as the first char
		wchar_t *tmpPtr = varStr;

		while(ch != L'>' && pStr <=startPos+strLen)
		{
			ch = parseQuote(*pStr);
			if(lastError != 0)
			{
				freeExpr();
				delete [] varStr;
				return NULL;
			}
			if(ch == L'\0')
			{
				freeExpr();
				delete [] varStr;
				lastError = -8;
				return NULL;
			}
			if(!( (ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9') || ch == L'_' || ch == L'-') && ch != L'>')
			{
				freeExpr();
				delete [] varStr;
				lastError = -14;
				return NULL;
			}

			if(ch != L'>')
			{
				*tmpPtr++ = *pStr;
				pStr++;
			}
		}
		if(pStr > startPos+strLen)
		{	// Error '<' not matched by end '>'
			freeExpr();
			delete [] varStr;
			lastError = -3;
			return NULL;
		}
		if(tmpPtr == varStr)
		{
			freeExpr();
			delete [] varStr;
			lastError = -12;
			return NULL;
		}
		*tmpPtr = L'\0';
	}
	if(*pStr == L'>')
	{
		// save the expression
		if(expr == NULL)
			expr = new exprStruct();
		expr->iCase = icase;
		expr->n = nVal;
		expr->negate = negateFlag;
		expr->pEnd = NULL;
		expr->pStart = NULL;
		expr->type = type;
		
		expr->varName = varStr;
		Expressions.push_back(expr);
		nExpressions++;
		return pStr;
	}
	else
	{
		freeExpr();
		lastError = -11; // end > tag not found where expected
		return NULL;
	}

	// Tag search complete
	//
	//////////////////////////////////////////

	// Check for varname

	// the next couple of chars should tell us the expression type, or we should fail

	// store the rest of the string in the token field

	// start tag parsing
	while(*pStr != L'>' && !quote && pStr <=startPos+strLen)
	{
		ch = parseQuote(*pStr);
		if(ch != L'\0' && lastError == 0)
		{	// ch is now our working char
			// wprintf(L"%c", ch);
		}
		else if(lastError != 0)
			return NULL;
		pStr++;
	}
	//wprintf(L"\n");
	if(pStr > startPos+strLen)
	{	// Error '<' not matched by end '>'
		lastError = -3;
		return NULL;
	}
	return pStr;
}

bool CStrMatch::freeExpressions()
{
	while(!Expressions.empty())
	{
		while(!Expressions.back()->token.empty())
		{
			delete [] Expressions.back()->token.back();
			Expressions.back()->token.pop_back();
		}
		if(Expressions.back()->varName != NULL)
			delete [] Expressions.back()->varName;
		
		delete Expressions.back();
		Expressions.pop_back();
	}
	nExpressions = 0;
	return true;
}

wchar_t *CStrMatch::GetErrorMessage()
{
	if(lastError == -1 && errorMsg != NULL)
		return errorMsg;
	if(errorMsg != NULL)
		delete [] errorMsg;

	if(lastError <= 0)
	{
		wchar_t *tmpMsg;
		if (lastError == 0)
			tmpMsg = L"StringMatch: Operation completed successfully.";
		else if (lastError == -1)
			tmpMsg = L"StringMatch: Operation complete, but no trace message set.";
		else if (lastError == -2)
			tmpMsg = L"StringMatch: Expression not set.";
		else if (lastError == -3)
			tmpMsg = L"StringMatch: Error parsing expression '<' not matched with '>'";
		else if (lastError == -4)
			tmpMsg = L"StringMatch: Unknown escape sequence entered in expression";
		else if (lastError == -5)
			tmpMsg = L"StringMatch: 0 is not a valid expression number";
		else if (lastError == -6)
			tmpMsg = L"StringMatch: Prefixing this expression with a number not valid";
		else if (lastError == -7)
			tmpMsg = L"StringMatch: Negating this expression is not allowed";
		else if (lastError == -8)
			tmpMsg = L"StringMatch: Escape character '\\' not allowed in an expression definition";
		else if (lastError == -9)
			tmpMsg = L"StringMatch: Invalid string, end of string is quoted";
		else if (lastError == -10)
			tmpMsg = L"StringMatch: Negate flag not allowed for <*> expressions";
		else if (lastError == -11)
			tmpMsg = L"StringMatch: Closing '>' tag not found where expected";
		else if (lastError == -12)
			tmpMsg = L"StringMatch: Variable separator '.' found, but no variable name specified";
		else if (lastError == -13)
			tmpMsg = L"StringMatch: Char sequence expression, without any chars for example <10[].var>";
		else if (lastError == -14)
			tmpMsg = L"StringMatch: Illegal character found in variable name, only 'a-z', 'A-Z', '0-9', '-', and '_' allowed";
		else if (lastError == -15)
			tmpMsg = L"StringMatch: Empty string found in string expression ie <!(str1|).var>";
		else if (lastError == -16)
			tmpMsg = L"StringMatch: IP expression defined, but no ip address specified";
		else if (lastError == -17)
			tmpMsg = L"StringMatch: Quote character '\\' not allowed in an IP address specification";
		else if (lastError == -18)
			tmpMsg = L"StringMatch: Unknown expression type defined";
		else if (lastError == -19)
			tmpMsg = L"StringMatch: Quote character '\\' not allowed in an oid specification";
		else if (lastError == -20)
			tmpMsg = L"StringMatch: Oid expression specified, but oid string is empty";
		else if (lastError == -21)
			tmpMsg = L"StringMatch: A string match was attempted with no expression defined";
		else if (lastError == -22)
			tmpMsg = L"StringMatch: Quoting is not allowed within an ip or oid expression string";
		else if (lastError == -23)
			tmpMsg = L"StringMatch: Invalid character found in oid or ip expression, only \"0-9\",\"-\",\"*\",\",\" and \".\" is allowed";
		
		
		errorMsg = new wchar_t[wcslen(tmpMsg)+1];
		wcscpy(errorMsg, tmpMsg);
	}
	else
	{
		wchar_t *tmpBuf = NULL;
		
		if(::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            lastError,
            0,
            (LPWSTR)&tmpBuf,
            0,
            NULL) == 0)
		{
			wchar_t *tmpMsg;
			tmpMsg = L"Unable to generate error message";
			errorMsg = new wchar_t[wcslen(tmpMsg)+1];
			wcscpy(errorMsg, tmpMsg);
		}
		else // success
		{
			size_t len = wcslen(tmpBuf);
			errorMsg = new wchar_t[len];
			wcsncpy(errorMsg, tmpBuf, len-2);
			*(errorMsg+len-2) = L'\0';
			LocalFree(tmpBuf);
		}
	}

	return errorMsg;
}

///////////////////////////////////////////////
//
// CVar definitions

void CVar::SetVar(wchar_t *name, wchar_t *value)
{
	if(name == NULL || value == NULL)
		return;
	
	// Loop through all variables to search for an
	// existing variable with name
	// if the variable exist, change its value, to value
	vector<SVarStruct*>::iterator i;
	for(i = varList.begin(); i!=varList.end(); i++)
	{
		if(_wcsicmp(name, (*i)->varName) == 0)
		{
			delete [] (*i)->varValue;
			(*i)->varValue = value;
			return;
		}
	}
	SVarStruct *newVar = new SVarStruct;
	
	newVar->varName = name;
	newVar->varValue = value;

	varList.push_back(newVar);

	if(nCount == 0)
		MoveFirst();
	nCount++;
	return;
}

wchar_t *CVar::GetValueByName(wchar_t *name)
{
	vector<SVarStruct*>::iterator i;
	for(i=varList.begin(); i!=varList.end(); i++)
	{
		if(_wcsicmp((*i)->varName, name) == 0)
			return (*i)->varValue;
	}
	return NULL;
}


//#endif