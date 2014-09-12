
// MonitorDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Monitor.h"
#include "pugixml.hpp"
#include "MonitorDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern Config g_client_config;
// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMonitorDlg 对话框

static pj_status_t init_param()
{
	pugi::xml_document doc;

    pugi::xml_parse_result result = doc.load_file("client.xml");
	RETURN_VAL_IF_FAIL(result, PJ_EINVAL);

	pugi::xml_node client = doc.child("client");
	g_client_config.client_id = atoi(client.attribute("client_id").value());
	g_client_config.local_ip = pj_str(strdup((char *)client.attribute("ip").value()));
	g_client_config.local_media_port = atoi(client.attribute("media_port").value());
	g_client_config.log_file_name = pj_str(strdup((char *)client.attribute("log_file_name").value()));
	g_client_config.tls_host = pj_str(strdup((char *)client.attribute("tls_host").value()));
	g_client_config.tls_port = atoi(client.attribute("tls_port").value());
	g_client_config.tls_uri = pj_str(strdup((char *)client.attribute("tls_uri").value()));
	g_client_config.rrtvms_fcgi_host = pj_str(strdup((char *)client.attribute("rrtvms_fcgi_host").value()));
	g_client_config.rrtvms_fcgi_port = atoi(client.attribute("rrtvms_fcgi_port").value());
	g_client_config.rrtvms_fcgi_uri = pj_str(strdup((char *)client.attribute("rrtvms_fcgi_uri").value()));

	return PJ_SUCCESS;
}

CMonitorDlg::CMonitorDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMonitorDlg::IDD, pParent)
	, is_draging_(PJ_FALSE)
	, draging_user_(nullptr)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMonitorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMonitorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZING()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_BN_CLICKED(IDC_BUTTON1, &CMonitorDlg::OnChangeLayout)
	ON_MESSAGE(WM_BEGINDRAGITEM, &CMonitorDlg::OnBeginDragItem)
	ON_MESSAGE(WM_ENDDRAGITEM, &CMonitorDlg::OnEndDragItem)
	ON_MESSAGE(WM_EXPANDEDROOM, &CMonitorDlg::OnLinkRoom)
	ON_MESSAGE(WM_EXPANDEDROOM, &CMonitorDlg::OnUnlinkRoom)
END_MESSAGE_MAP()

// CMonitorDlg 消息处理程序
static ScreenMgr *g_screen_mgr = NULL;
BOOL CMonitorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	CRect rect;
	GetClientRect(&rect);

	pj_int32_t width = abs(rect.bottom - rect.top);
	pj_int32_t height = abs(rect.right - rect.left);

	SDL_Init( SDL_INIT_VIDEO );
	av_register_all();
	pj_init();

	pj_status_t status;
	status = init_param();
	pj_assert(status == PJ_SUCCESS);

	g_screen_mgr = new ScreenMgr(this, 10, g_client_config.local_ip, g_client_config.local_media_port);
	pj_assert(g_screen_mgr != nullptr);

	status = g_screen_mgr->Prepare(g_client_config.log_file_name);
	pj_assert(status == PJ_SUCCESS);

	status = g_screen_mgr->Launch();
	pj_assert(status == PJ_SUCCESS);

	g_screen_mgr->Adjest(width, height);

	MoveWindow(CRect(0, 0, width, height));
	ShowWindow(SW_SHOW);

	GetDlgItem(IDC_BUTTON1)->ShowWindow(SW_HIDE);
	PJ_LOG(1, (__FILE__, "Client start....."));

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMonitorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}

	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMonitorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMonitorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMonitorDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	pj_assert(lpMMI != NULL);
	lpMMI->ptMinTrackSize.x = g_screen_mgr->GetDefaultResolution().x;
	lpMMI->ptMinTrackSize.y = g_screen_mgr->GetDefaultResolution().y;

	CDialog::OnGetMinMaxInfo(lpMMI);
}

void CMonitorDlg::OnSizing(UINT nSide, LPRECT lpRect)
{
	g_screen_mgr->GetSuitedSize(lpRect);

	CDialogEx::OnSizing(nSide, lpRect);
}

void CMonitorDlg::OnSize(UINT nType, int cx, int cy)
{
	g_screen_mgr->Adjest( cx, cy );

	CDialogEx::OnSize(nType, cx, cy);
}

void CMonitorDlg::OnChangeLayout()
{
	struct resolution
	{
		pj_uint32_t width;
		pj_uint32_t height;
		enum_screen_mgr_resolution_t res;
	} ress[4] = 
	{
		{ 400, 400, SCREEN_RES_1x1 },
		{ 200, 200, SCREEN_RES_2x2 },
		{ 100, 100, SCREEN_RES_1x5 },
		{ 100, 100, SCREEN_RES_3x3 },
	};

	static enum_screen_mgr_resolution_t g_res_type = SCREEN_RES_1x1;

	g_screen_mgr->ChangeLayout(ress[g_res_type].res);

	g_res_type = (enum_screen_mgr_resolution_t)(( g_res_type + 1 ) % 4);
}

LRESULT CMonitorDlg::OnBeginDragItem(WPARAM wParam, LPARAM lParam)
{
	TitleRoom *title_room = reinterpret_cast<TitleRoom *>(wParam);
	User *user = reinterpret_cast<User *>(lParam);
	RETURN_VAL_IF_FAIL(user, true);

	is_draging_ = PJ_TRUE;
	draging_rooom_ = title_room;
	draging_user_ = user;

	return true;
}

LRESULT CMonitorDlg::OnEndDragItem(WPARAM wParam, LPARAM lParam)
{
	Screen *screen = reinterpret_cast<Screen *>(lParam);
	RETURN_VAL_IF_FAIL((screen && draging_rooom_ && draging_user_ && is_draging_), true);

	g_screen_mgr->LinkScreenUser(screen, draging_rooom_, draging_user_);

	is_draging_ = PJ_FALSE;
	draging_rooom_ = nullptr;
	draging_user_ = nullptr;

	return true;
}

LRESULT CMonitorDlg::OnLinkRoom(WPARAM wParam, LPARAM lParam)
{
	TitleRoom *title_room = (TitleRoom *)lParam;
	RETURN_VAL_IF_FAIL(title_room, true);

	title_room->id_ = 462728;

	g_screen_mgr->OnLinkRoom(title_room);

	return true;
}

LRESULT CMonitorDlg::OnUnlinkRoom(WPARAM wParam, LPARAM lParam)
{
	User *user = reinterpret_cast<User *>(wParam);
	Screen *screen = reinterpret_cast<Screen *>(lParam);
	RETURN_VAL_IF_FAIL(user && screen, true);

	/*g_screen_mgr->OnUnlinkRoom(title_room);*/

	return true;
}