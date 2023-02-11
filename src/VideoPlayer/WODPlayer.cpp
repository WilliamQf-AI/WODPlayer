
#include "pch.h"
#include "resource.h"
#include "database/database_helper.h"
//#include "VideoPlayer/VLCPlayer.h"

extern VideoPlayer* initVidePlayerImpl(WODPlayer* xpp, const TCHAR* pluginName);

WODPlayer::WODPlayer() {
	_timeMarked = -1;
	_handleMsg = true;
}

void WODPlayer::newVideoView()
{
	if (_mMediaPlayer)
	{
		delete _mMediaPlayer;
		_mMediaPlayer = 0;
		if(_hPlayer)
		{
			CloseWindow(_hPlayer);
			_hPlayer = 0;
		}
	}
	_mMediaPlayer = initVidePlayerImpl(this, L"MFExternalPlayer.dll");
	//_mMediaPlayer = initVidePlayerImpl(this, L"VLCExternalPlayer.dll");
	//_mMediaPlayer = initVidePlayerImpl(this, L"XunLeiExternalPlayer\\XunLeiExternalPlayer.dll");
	if (_mMediaPlayer)
	{
		_hPlayer = _mMediaPlayer->getHWND();
	}
}

void WODPlayer::Release()
{
	if(_mMediaPlayer) {
		_mMediaPlayer->Release();
		delete _mMediaPlayer;
	}
}

//class DummyPlayer : public VideoPlayer
//{
//public:
//	void Play(){}
//	void Stop(){}
//	void Pause(){}
//	bool IsPlaying(){}
//	bool IsPaused(){}
//	long GetPosition(){}
//	void SetPosition(long pos){}
//	long GetDuration(){}
//	bool PlayVideoFile(const TCHAR* path){}
//};

bool WODPlayer::PlayVideoFile(const TCHAR* path)
{
	bool ret = false;
	if (!_mMediaPlayer)
	{
		newVideoView();
	}
	//ASSERT(_mMediaPlayer);
	if (_mMediaPlayer)
	{
		ret = _mMediaPlayer->PlayVideoFile(path);
	}
	if (ret)
	{
		_currentPath = path;
		QkString tmp;
		const char* pStr;
		if(_currentPath.Find('\"')) {
			tmp = _currentPath;
			tmp.Replace(L"\"", L"\"\"");
			pStr = tmp.GetData(threadBuffer);
		} else {
			pStr = _currentPath.GetData(threadBuffer);
		}
		_timeMarked = _app->_db->GetBookMarks(pStr, _bookmarks);
	}
	QkString filePath = path;
	PutProfString("file", filePath.GetData(threadBuffer));
	return false;
}

bool WODPlayer::AddBookmark()
{
	if(_mMediaPlayer) {
		int pos = _mMediaPlayer->GetPosition();
		int duration = _mMediaPlayer->GetDuration();
		//LogIs(2, L"%s", (LPCWSTR)_currentPath);
		_app->_db->AddBookmark(_currentPath.GetData(threadBuffer), 0, _timeMarked, pos, duration, 0);

		return true;
	}
	return false;
}

void WODPlayer::SetPos(RECT rc, bool bNeedInvalidate)
{
	if(_hPlayer) {
		//::MoveWindow(_hPlayer, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, bNeedInvalidate);
		//::SetWindowPos(_hPlayer, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, bNeedInvalidate);

		if(_mMediaPlayer->_resX && _mMediaPlayer->_resY) 
		//if(0) 
		{
			float width = rc.right - rc.left;
			float height = rc.bottom-rc.top;
			float ratio = min(width/_mMediaPlayer->_resX, height/_mMediaPlayer->_resY);
			int w = _mMediaPlayer->_resX*ratio;
			int h = _mMediaPlayer->_resY*ratio;
			_exRect.left = (width-w)/2;
			_exRect.top = max(0, (height-h)/2);
			_exRect.right = _exRect.left+w;
			_exRect.bottom = _exRect.top+h;
			::SetWindowPos(_mMediaPlayer->getHWND(), HWND_TOP, 
				_exRect.left,  _exRect.top, 
				w,  h, 
				SWP_SHOWWINDOW);
		} 
		else 
		{
			::SetWindowPos(_mMediaPlayer->getHWND(), HWND_TOP, 
				rc.left, 
				rc.top, 
				//1*(rect.right - rect.left), 
				1*(rc.right - rc.left), 
				rc.bottom-rc.top, 
				SWP_SHOWWINDOW);
		}

		// 
		//LogIs(2, "youre   %ld", GetHWND());
	}
	__super::SetPos(rc, bNeedInvalidate);

	//::SendMessage(GetHWND(), MM_PREPARED, 0, 0);
}

bool WODPlayer::IsMediaPlayerWindow(HWND hwnd)
{
	return hwnd==_hPlayer||IsChild(_hPlayer, hwnd);
}

HWND WODPlayer::GetMediaPlayerHWND()
{
	return _hPlayer;
}

void WODPlayer::Toggle()
{
	if (_mMediaPlayer)
	{
		if (_isPlaying)
		{
			_mMediaPlayer->Pause();
		}
		else 
		{
			_mMediaPlayer->Play();
		}
		MarkPlaying(!_isPlaying);
	}
}

void WODPlayer::MarkPlaying(bool playing)
{
	if (_isPlaying!=playing)
	{
		_isPlaying = playing;
		_app->MarkPlaying(playing);
	}
}

bool WODPlayer::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& ret)
{
	switch (uMsg)
	{
		case MM_PREPARED:
		{
			//LogIs(2, "MPM_PREPARED %d\n", wParam);
			_seekbar.SetMax(wParam);
			MarkPlaying(true);
			//if(_app->_WndOp==1)
			{
				SetPos(GetPos());
			}
		} return 1;
		case MM_STOPPED:
		{
			MarkPlaying(false);
		} return 1;
		case WM_PAINT:
		{
			//if (WS_EX_LAYERED == (WS_EX_LAYERED & GetWindowLong(hWnd, GWL_EXSTYLE))) break;;
			//RECT rcClient;
			//::GetClientRect(GetHWND(), &rcClient);

			PAINTSTRUCT ps = { 0 };
			HDC hdc = ::BeginPaint(GetHWND(), &ps);

			HBRUSH hbrush1 = CreateSolidBrush(~TransparentKey);
			FillRect(hdc, &_exRect, hbrush1);

			//::GetClientRect(_hPlayer, &_exRect);
			::ExcludeClipRect(hdc, _exRect.left, _exRect.top, _exRect.right, _exRect.bottom);
			//::ExcludeClipRect(hdc, 0, 0, 100, 100);

			RECT rect = ps.rcPaint; 
			//rect.bottom -= 50;

			//rect.right = rect.left+(rect.right-rect.left)/2;

			HBRUSH hbrush = CreateSolidBrush(TransparentKey);

			FillRect(hdc, &rect, hbrush);



			::EndPaint(GetHWND(), &ps);


			//PAINTSTRUCT ps = { 0 };
			//::BeginPaint(GetHWND(), &ps);
			//CRenderEngine::DrawColor(, ps.rcPaint, 0xFF000000);
			//::EndPaint(GetHWND(), &ps);

			// todo clean up

			return 0;
		} return 1;
	}
	return 0;
}