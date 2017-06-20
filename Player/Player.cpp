#include "stdafx.h"
#include "Player.h"

typedef int (*LPMediaOpen)(const char *filename);
typedef int (*LPMediaPlay)(HWND hWnd,RECT rcPos);
typedef int (*LPMediaClose)();

LPMediaOpen pMediaOpen = NULL;
LPMediaPlay pMediaPlay = NULL;
LPMediaClose pMediaClose = NULL;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH: {
		HMODULE hFFPlay = NULL;
#ifdef _DEBUG
		hFFPlay = ::LoadLibrary(_T("FFPlay_d.dll"));
#else
		hFFPlay = ::LoadLibrary(_T("FFPlay.dll"));
#endif
		if( hFFPlay != NULL ) {
			pMediaOpen = (LPMediaOpen)::GetProcAddress(hFFPlay, "media_open");
			pMediaPlay = (LPMediaPlay)::GetProcAddress(hFFPlay, "media_play");
			pMediaClose = (LPMediaClose)::GetProcAddress(hFFPlay, "media_close");
		}
		break;
							 }
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

extern "C" PLAYER_API DuiLib::CControlUI* CreateControl(LPCTSTR pstrClass)
{
	if (_tcsicmp(pstrClass, _T("Player")) == 0)
	{
		return new DuiLib::CPlayerUI();
	}
	return	NULL;
}

namespace DuiLib
{
	class CPlayerWnd : public CWindowWnd
	{
	public:
		CPlayerWnd();
		~CPlayerWnd();

		void Init(CPlayerUI* pOwner);
		LRESULT Play(LPCSTR filepath);
		RECT CalPos();

		LPCTSTR GetWindowClassName() const;
		void OnFinalMessage(HWND hWnd);

		bool Open(LPCSTR filepath);
		bool Play();
		bool Stop();

	protected:
		CPaintManagerUI m_Manager;
		CPlayerUI* m_pOwner;
	};

	CPlayerWnd::CPlayerWnd() : m_pOwner(NULL)
	{
	}

	CPlayerWnd::~CPlayerWnd()
	{
	}

	void CPlayerWnd::Init(CPlayerUI* pOwner)
	{
		m_pOwner = pOwner;
		RECT rcPos = CalPos();
		UINT uStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		Create(m_pOwner->GetManager()->GetPaintWindow(),_T("UIPlayer"),uStyle,0,rcPos);
		::ShowWindow(m_hWnd, pOwner->IsVisible() ? SW_SHOW : SW_HIDE);
	}

	RECT CPlayerWnd::CalPos()
	{
		CDuiRect rcPos = m_pOwner->GetPos();
		CControlUI* pParent = m_pOwner;
		RECT rcParent;
		while( pParent = pParent->GetParent() ) {
			if( !pParent->IsVisible() ) {
				rcPos.left = rcPos.top = rcPos.right = rcPos.bottom = 0;
				break;
			}
			rcParent = pParent->GetClientPos();
			if( !::IntersectRect(&rcPos, &rcPos, &rcParent) ) {
				rcPos.left = rcPos.top = rcPos.right = rcPos.bottom = 0;
				break;
			}
		}

		return rcPos;
	}

	LPCTSTR CPlayerWnd::GetWindowClassName() const
	{
		return _T("PlayerWnd");
	}

	void CPlayerWnd::OnFinalMessage(HWND hWnd)
	{
		if(pMediaClose){
			pMediaClose();
		}

		m_pOwner->Invalidate();
		m_pOwner->m_pWindow = NULL;

		delete this;
	}

	bool CPlayerWnd::Open(LPCSTR filepath)
	{
		if(pMediaOpen){
			pMediaOpen(filepath);
		}
		return true;
	}

	bool CPlayerWnd::Play()
	{
		if(pMediaPlay){
			pMediaPlay(m_hWnd,CalPos());
		}
		return true;
	}

	bool CPlayerWnd::Stop()
	{
		::SendMessage(m_hWnd, WM_CLOSE, 0, 0);
		if(pMediaClose){
			pMediaClose();
		}
		return true;
	}


	/////////////////////////////////////////////////////////////////////////////////////
	//

	CPlayerUI::CPlayerUI() : m_pWindow(NULL)
	{
	}

	CPlayerUI::~CPlayerUI()
	{
		if( m_pWindow != NULL ) {
			::DestroyWindow(*m_pWindow);
			delete m_pWindow;
			m_pWindow =	NULL;
		}
	}

	LPCTSTR CPlayerUI::GetClass() const
	{
		return _T("CPlayerUI");
	}

	LPVOID CPlayerUI::GetInterface(LPCTSTR pstrName)
	{
		if( _tcscmp(pstrName, _T("Player")) == 0 ) return static_cast<CPlayerUI*>(this);
		return CControlUI::GetInterface(pstrName);
	}

	HWND CPlayerUI::GetNativeWindow() const
	{
		if (m_pWindow) return m_pWindow->GetHWND();
		return NULL;
	}

	bool CPlayerUI::Activate()
	{
		if( !IsVisible() ) return false;
		if( !IsEnabled() ) return false;

		if(m_pWindow == NULL){
			m_pWindow = new CPlayerWnd();
			m_pWindow->Init(this);
		}
		return true;
	}

	void CPlayerUI::DoEvent(TEventUI& event)
	{
		CControlUI::DoEvent(event);
	}

	void CPlayerUI::SetPos(RECT rc, bool bNeedInvalidate)
	{
		CControlUI::SetPos(rc, bNeedInvalidate);
		if( m_pWindow != NULL ) {
			::SetWindowPos(m_pWindow->GetHWND(), NULL, rc.left, rc.top, rc.right - rc.left,
				rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOACTIVATE);
		}
	}

	void CPlayerUI::SetVisible(bool bVisible)
	{
		if( m_bVisible == bVisible ) return;
		CControlUI::SetVisible(bVisible);
		if( IsVisible() && m_pWindow != NULL )
			::ShowWindow(*m_pWindow, IsVisible() ? SW_SHOW : SW_HIDE);
	}

	void CPlayerUI::SetInternVisible(bool bVisible)
	{
		if( m_bVisible == bVisible ) return;
		CControlUI::SetInternVisible(bVisible);
		if( IsVisible() && m_pWindow != NULL )
			::ShowWindow(*m_pWindow, IsVisible() ? SW_SHOW : SW_HIDE);
	}

	bool CPlayerUI::Open(LPCSTR filepath)
	{
		Activate();
		if(m_pWindow){
			return m_pWindow->Open(filepath);
		}

		return false;
	}

	bool CPlayerUI::Play()
	{
		Activate();
		if(m_pWindow){
			SetVisible(true);
			return m_pWindow->Play();
		}
		return false;
	}

	bool CPlayerUI::Stop()
	{
		if(m_pWindow){
			SetVisible(false);
			return m_pWindow->Stop();
		}
		return false;
	}
}
