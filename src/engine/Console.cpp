/**
\author		Korotkov Andrey aka DRON
\date		07.10.2012 (c)Korotkov Andrey

This file is a part of DGLE project and is distributed
under the terms of the GNU Lesser General Public License.
See "DGLE.h" for more details.
*/

#include "Console.h"

#ifdef PLATFORM_WINDOWS
#include "platform\windows\ConsoleWindow.h"
#endif

using namespace std;

#define CON_SPLIT_TWO_PARAMS(params)\
	string::size_type pos = params.find_first_of(' ');\
	string par1(params.substr(0, pos)), par2(params.substr(pos + 1, params.size() - pos - 1));

class CEvConsoleWrite : public IEvConsoleWrite
{
	string _txt;
	bool _bToPrev;

public:

	CEvConsoleWrite(const string &txt, bool toPrev):_txt(txt), _bToPrev(toPrev) {}

	DGLE_RESULT DGLE_API GetText(char *pcTxt, uint &uiCharsCount, bool &bToPrevLine)
	{
		bToPrevLine = _bToPrev;

		if (pcTxt == NULL)
		{
			uiCharsCount = _txt.size() + 1;
			return S_OK;
		}

		if (uiCharsCount <= _txt.size())
		{
			uiCharsCount = _txt.size() + 1;
			strcpy(pcTxt, "");
			return E_INVALIDARG;
		}

		strcpy(pcTxt, _txt.c_str());

		return S_OK;
	}

	DGLE_RESULT DGLE_API GetEventType(E_EVENT_TYPE &eEvType)
	{
		eEvType = ET_ON_CONSOLE_WRITE;
		return S_OK;
	}

	DGLE_RESULT DGLE_API GetUnknownEventType(uint &uiUnknEvType)
	{
		uiUnknEvType = -1;
		return S_FALSE;
	}

	IDGLE_BASE_IMPLEMENTATION(IEvConsoleWrite, INTERFACE_IMPL(IBaseEvent, INTERFACE_IMPL_END))
};

CConsole::CConsole(uint uiInsIdx):
CInstancedObj(uiInsIdx), _pConsoleWindow(NULL), _iPrevMarker(0)
{
	RegComProc("terminate", "Terminates application (causes system to hardly terminate application process). Use it only if application is not responding.", &_s_Terminate, (void*)this);
	RegComProc("help", "", &_s_Help, this);
	RegComProc("cmdlist", "Outputs list of available console commands.", &_s_Cmdlist, (void*)this);
	RegComProc("clear", "Clears all text in console.", &_s_Clear, (void*)this);
	RegComProc("save", "Saves current console output to file. When filename is not specified, output saves to \"console.txt\".\nUsage: \"save [filename]\"", &_s_Save, (void*)this);
	RegComProc("con_resetpos", "Resets and recalculate console window screen position and size. Useful when console windows is out of screen area.", &_s_ResetPos, (void*)this);
	RegComProc("con_show", "Shows console window, if is hidden.", &_s_Show, (void*)this);
	RegComProc("con_hide", "Hides console window.", &_s_Hide, (void*)this);
	RegComProc("con_pos", "Changes console window position.\nUsage: \"con_pos <x coord> <y coord>\"", &_s_SetPos, (void*)this);
	RegComProc("con_size", "Changes console window size.\nUsage: \"con_size <width value> <height value>\"", &_s_SetSize, (void*)this);

	_pConsoleWindow = new CConsoleWindow();
	_pConsoleWindow->InitWindow(!(EngineInstance(InstIdx())->eGetEngFlags & GEF_FORCE_SINGLE_THREAD), &_s_OnConWindowEvent, this);
}

CConsole::~CConsole()
{
	for(uint i = 0; i<_commands.size(); ++i)
	{
		delete[] _commands[i].pcName;
		delete[] _commands[i].pcHelp;
	}

	_commands.clear();

	_pConsoleWindow->Free();
}

void CConsole::_Help(const char* pcParam)
{
	string par(pcParam);

	if (par == "" || par == "help")
	{
		Write(	"Print \"cmdlist\" for list of available console commands.\n"
				"Print \"help <command>\" for specific command help.\n"
				"Press \"Tab\" key for command autocompletion.\n"
				"Press \"Up arrow\" key for previous command.\n"
				"Press \"Down arrow\" key for next command.");
	} 
	else
	{
		if (par[par.length() - 1] == ' ')
			par.erase(par.length() - 1, 1);

		for (size_t i = 0; i < _commands.size(); ++i)
		 if (par == string(_commands[i].pcName))
		 {
			 if (strlen(_commands[i].pcHelp) == 0)
				Write(string("Help for command \"" + par + "\" is not presented.").c_str());
			 else
				Write(_commands[i].pcHelp);
			 
			 return;
		 }

		 Write(string("Command \"" + par + "\" not found.").c_str());
	}
}

void CConsole::Exec(const char* pcCommand)
{
	string command(pcCommand);

	Write((">" + command).c_str());
	
	_ProcessConCmd(command);

	_prevCommands.push_back(command);
	_iPrevMarker = _prevCommands.size();
}

void CConsole::_Cmdlist()
{
	string lst = "----------Commands List----------\n";

	if (!_commands.empty())
	{
		for (size_t i = 0; i < _commands.size(); ++i)
			lst += " >" + string(_commands[i].pcName) + "\n";
		
		lst += string("------" + UIntToStr((uint)_commands.size()) + " registered commands-----" + (_commands.size() < 100 ? "-" : "") + "\n");
		
		Write(lst.c_str());
	}
}

void CConsole::_Save(const string &strFileName)
{
	uint size;
	_pConsoleWindow->GetConsoleTxt(NULL, size);

	char *p_txt = new char[size];

	_pConsoleWindow->GetConsoleTxt(p_txt, size);

	fstream file;
	file.setf(ios_base::right, ios_base::adjustfield);

	if (strFileName.empty())
		file.open("console.txt", ios::out | ios::trunc);
	else
		file.open(strFileName.c_str(), ios::out | ios::trunc);

	if (!file.is_open())
	{
		Write("Failed to open output file for writing.");
		return;
	}

	file << p_txt << endl;

	file.close();
}

void CConsole::EnterThreadSafeSection()
{
	_pConsoleWindow->EnterThreadSafeSection();
}

void CConsole::LeaveThreadSafeSection()
{
	_pConsoleWindow->LeaveThreadSafeSection();
}

void CConsole::_OnCmdPrev()
{
	--_iPrevMarker;

	if (_iPrevMarker < 0)
		_iPrevMarker = 0;

	if ((uint)_iPrevMarker < _prevCommands.size())
		_pConsoleWindow->SetEditTxt(_prevCommands[_iPrevMarker].c_str());
}

void CConsole::_OnCmdNext()
{
	++_iPrevMarker;

	if ((uint)_iPrevMarker >= _prevCommands.size())
	{
		_iPrevMarker = _prevCommands.size() - 1;
		_pConsoleWindow->SetEditTxt("");
	}
	else
		_pConsoleWindow->SetEditTxt(_prevCommands[_iPrevMarker].c_str());
}

bool CConsole::_ProcessConCmd(const std::string &command)
{
	string cmd, param;
	
	string::size_type pos = command.find_first_of(' ');
		
	if (pos != string::npos)
	{
		param = command.substr(pos + 1, command.size() - pos - 1);
		cmd = command.substr(0, pos);
	}
	else
		cmd = command;
	
	for (size_t i = 0; i < _commands.size(); ++i)
		if (cmd == _commands[i].pcName)
		{
			if (_commands[i].piValue == NULL)
			{
				_pConsoleWindow->EnterThreadSafeSection();

				(*_commands[i].pProc)(_commands[i].pParametr, param.c_str());
				
				_pConsoleWindow->LeaveThreadSafeSection();
			}
			else
			{
				if (param.empty())
				{
					Write((ToUpperCase(cmd) + " current value is " + IntToStr(*_commands[i].piValue) + " .\n"
						"Value may vary from " + IntToStr(_commands[i].iMinValue) + " up to " + IntToStr(_commands[i].iMaxValue) + ".").c_str());
				}
				else
				{
					int t = StrToInt(param);

					if (t == 0 && param != "0")
						Write(("\"" + param + "\" is not a valid integer value.").c_str());
					else
						if (t < _commands[i].iMinValue || t > _commands[i].iMaxValue)
							Write(("Value may vary from " + IntToStr(_commands[i].iMinValue) + " up to " + IntToStr(_commands[i].iMaxValue) + ".").c_str());
						else
						{
							_pConsoleWindow->EnterThreadSafeSection();
						
							*_commands[i].piValue = t;
						
							if (_commands[i].pProc != NULL)
								(*_commands[i].pProc)(_commands[i].pParametr, param.c_str());

							_pConsoleWindow->LeaveThreadSafeSection();

							Write((ToUpperCase(cmd) + " is set to " + IntToStr(t) + ".").c_str());
						}
				}
			}

			return true;
		}

	Write("Unknown command.");

	return false;
}

void CConsole::_OnCmdComplete(const char *pcParam)
{
	string cmds = "----\n";
	int count = 0, idx = 0;
	
	for (size_t i = 0; i < _commands.size(); ++i)
	{	
		bool flag = true;
	
		if (strlen(pcParam) > strlen(_commands[i].pcName))
			flag = false;
		else
			for (uint j = 0; j < strlen(pcParam); ++j)
				if (pcParam[j] != _commands[i].pcName[j])
				{
					flag = false;
					break;
				}

		if (flag)
		{
			++count;
			idx = (int)i;
			cmds += " >"+string(_commands[i].pcName) + "\n";
		}
	}
		cmds+="----";
		
		if (count == 1) 
			_pConsoleWindow->SetEditTxt((string(_commands[idx].pcName) + " ").c_str());
		else
			if (count > 1)
				Write(cmds.c_str());
}

bool CConsole::UnRegCom(const char *pcName)
{
	for (size_t i = 0; i < _commands.size(); ++i)
		if (strcmp(_commands[i].pcName, pcName) == 0)
		{
			delete[] _commands[i].pcName;
			delete[] _commands[i].pcHelp;
			
			_commands.erase(_commands.begin() + i);
			
			return true;
		}

	return false;
}

void CConsole::RegComProc(const char *pcName, const char *pcHelp, void (DGLE_API *pProc)(void *pParametr, const char *pcParam), void *pParametr)
{
	TConEntry t;
	
	t.pcName = new char[strlen(pcName) + 1];
	strcpy(t.pcName, pcName);
	
	t.pcHelp = new char[strlen(pcHelp) + 1];
	strcpy(t.pcHelp, pcHelp);
	
	t.pProc	= pProc;
	t.piValue = NULL;
	t.iMaxValue	= 0;
	t.iMinValue	= 0;
	t.pParametr	= pParametr;
	
	_commands.push_back(t);
	
	sort(_commands.begin(), _commands.end());
}

void CConsole::RegComValue(const char *pcName, const char *pcHelp, int *piValue, int iMin, int iMax, void (DGLE_API *pProc)(void *pParametr, const char *pcParam), void *pParametr) 
{
	TConEntry t;
	
	t.pcName = new char[strlen(pcName) + 1];
	strcpy(t.pcName, pcName);
	
	t.pcHelp = new char[strlen(pcHelp) + 1];
	strcpy(t.pcHelp, pcHelp);
	
	t.pProc	= pProc;
	t.piValue = piValue;
	t.iMaxValue	= iMax;
	t.iMinValue	= iMin;
	t.pParametr	= pParametr;
	
	_commands.push_back(t);	
	
	sort(_commands.begin(), _commands.end());
}

void CConsole::_SetPos(const char* pcParam)
{
	string par(pcParam);
	
	int x, y, w, h;

	_pConsoleWindow->GetSizeAndPos(x, y, w, h);
	
	if (par.size() == 0)
		Write(string("Current console position is left: " + IntToStr(x) + " top: " + IntToStr(y) + " .").c_str());
	else
	{
		CON_SPLIT_TWO_PARAMS(par);

		x = StrToInt(par1);
		
		if (x == 0 && par1 != "0") 
		{
			Write(string("\"" + par1 + "\" is not a valid integer value.").c_str());
			return;
		}

		y = StrToInt(par2);
		
		if (y == 0 && par2 != "0") 
		{
			Write(string("\"" + par2 + "\" is not a valid integer value.").c_str());
			return;
		}

		_pConsoleWindow->SetSizeAndPos(x, y, w, h);

		Write(string("Console position is set to left: " + IntToStr(x) + " top: " + IntToStr(y) + " .").c_str());
	}
}

void CConsole::_SetSize(const char *pcParam)
{
	string par(pcParam);
	
	int x, y, w, h;

	_pConsoleWindow->GetSizeAndPos(x, y, w, h);

	if (par.size() == 0)
		Write(string("Current console size is width: " + IntToStr(w) + " height: " + IntToStr(h) + " .").c_str());
	else
	{
		CON_SPLIT_TWO_PARAMS(par);

		w = StrToInt(par1);
		
		if (w == 0 && par1 != "0") 
		{
			Write(string("\"" + par1 + "\" is not a valid integer value.").c_str());
			return;
		}

		h = StrToInt(par2);
		
		if (h == 0 && par2 != "0") 
		{
			Write(string("\"" + par2 + "\" is not a valid integer value.").c_str());
			return;
		}

		_pConsoleWindow->SetSizeAndPos(x, y, w, h);

		Write(string("Console size is set to width: " + IntToStr(w) + " height: " + IntToStr(h) + " .").c_str());
	}
}

void CConsole::ResetWinPos()
{
	_pConsoleWindow->ResetSizeAndPos();
}

void CConsole::Visible(bool IsVisible)
{
	_pConsoleWindow->Visible(IsVisible);
}

TWinHandle CConsole::GetWindowHandle()
{
	TWinHandle handle;
	_pConsoleWindow->GetWindowHandle(handle);
	
	return handle;
}

uint32 CConsole::GetThreadId()
{
	uint32 thread_id;
	_pConsoleWindow->GetThreadId(thread_id);

	return thread_id;
}

void CConsole::Write(const std::string &strTxt, bool bToPrevLine)
{
	string txt(strTxt);

	if (Core())
		Core()->CastEvent(ET_ON_CONSOLE_WRITE, &CEvConsoleWrite(txt, bToPrevLine));

	for (size_t i = 0; i < txt.size(); ++i)
		if (txt[i] == '\n')
		{
			txt.insert(i, "\r");
			++i;
		}

	_pConsoleWindow->OutputTxt(txt.c_str(), bToPrevLine);
}

void DGLE_API CConsole::_s_Hide(void *pParametr, const char *pcParam)
{
	if (strlen(pcParam) != 0)
		PTHIS(CConsole)->Write("No parametrs expected.");
	else
		PTHIS(CConsole)->Visible(false);
}

void DGLE_API CConsole::_s_Show(void *pParametr, const char *pcParam)
{
	if (strlen(pcParam) != 0)
		PTHIS(CConsole)->Write("No parametrs expected.");
	else
		PTHIS(CConsole)->Visible(true);
}

void DGLE_API CConsole::_s_Clear(void *pParametr, const char *pcParam)
{
	if (strlen(pcParam) != 0)
		PTHIS(CConsole)->Write("No parametrs expected.");
	else
		PTHIS(CConsole)->_pConsoleWindow->Clear();
}

void DGLE_API CConsole::_s_Save(void *pParametr, const char *pcParam)
{
	PTHIS(CConsole)->_Save(string(pcParam));
}

void DGLE_API CConsole::_s_Terminate(void *pParametr, const char *pcParam)
{
	if (strlen(pcParam) != 0)
		PTHIS(CConsole)->Write("No parametrs expected.");
	else
		Terminate();
}

void DGLE_API CConsole::_s_SetPos(void *pParametr, const char *pcParam)
{
	PTHIS(CConsole)->_SetPos(pcParam);
}

void DGLE_API CConsole::_s_SetSize(void *pParametr, const char *pcParam)
{
	PTHIS(CConsole)->_SetSize(pcParam);
}

void DGLE_API CConsole::_s_ResetPos(void *pParametr, const char *pcParam)
{
	if (strlen(pcParam) != 0)
		PTHIS(CConsole)->Write("No parametrs expected.");
	else
		PTHIS(CConsole)->_pConsoleWindow->ResetSizeAndPos();
}

void DGLE_API CConsole::_s_Cmdlist(void *pParametr, const char *pcParam)
{
	if (strlen(pcParam) != 0)
		PTHIS(CConsole)->Write("No parametrs expected.");
	else
		PTHIS(CConsole)->_Cmdlist();
}

void DGLE_API CConsole::_s_Help(void *pParametr, const char *pcParam)
{
	PTHIS(CConsole)->_Help(pcParam);
}

void DGLE_API CConsole::_s_OnConWindowEvent(CConsole *pConsole, E_CONSOLE_WINDOW_EVENT eEventType, const char *pcCommand)
{
	switch (eEventType)
	{
	case CWE_EXECUTE_COMMAND: pConsole->Exec(pcCommand); break;
	case CWE_COMPLETE_COMMAND: pConsole->_OnCmdComplete(pcCommand); break;
	case CWE_PREVIOUS_COMMAND: pConsole->_OnCmdPrev(); break;
	case CWE_NEXT_COMMAND: pConsole->_OnCmdNext(); break;
	}
}
