#include "stdafx.h"
#include "Title.h"

BEGIN_MESSAGE_MAP(Title, CTreeCtrl)
	ON_WM_CONTEXTMENU()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, &Title::OnItemExpanded)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG,    &Title::OnTvnBeginDrag)
	ON_NOTIFY_REFLECT(NM_RCLICK,       &Title::OnRightButtonClick)
	ON_COMMAND_RANGE(IDC_MENU_LOOKUP, IDC_MENU_LOOKUP, &Title::OnLookUpNode)
END_MESSAGE_MAP()

Title::Title(pj_uint32_t id, const pj_str_t &name, order_t order)
	: CTreeCtrl()
	, Node(id, name, order, 0, TITLE)
{
}

void Title::PreSubclassWindow()
{
   CTreeCtrl::PreSubclassWindow();
   EnableToolTips(TRUE);
}

pj_status_t Title::Prepare(const CWnd *wrapper, pj_uint32_t uid)
{
	BOOL result;
	result = Create(WS_VISIBLE | WS_TABSTOP | WS_CHILD | WS_BORDER
		| TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES,
		CRect(0, 0, 0, 0), (CWnd *)wrapper, uid);
	RETURN_VAL_IF_FAIL(result, PJ_EINVAL);

	return PJ_SUCCESS;
}

pj_status_t Title::Launch()
{
	return PJ_SUCCESS;
}

void Title::OnDestory()
{
}

void Title::AddNode(pj_uint32_t id, const pj_str_t &name, pj_uint32_t order, pj_uint32_t usercount)
{
	DelNodeOrRoom(id, *this);  // Make sure it wasn't exist!

	node_map_t::mapped_type node = new TitleNode(id, name, order, usercount);
	pj_assert(node);
	AddNodeOrRoom(id, node, *this, TVI_ROOT);
}

void Title::MoveToRect(const CRect &rect)
{
	MoveWindow(rect);
	ShowWindow(SW_SHOW);
}

void Title::HideWindow()
{
	ShowWindow(SW_HIDE);
}

void Title::OnItemExpanded(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	HTREEITEM pTreeItem = reinterpret_cast<HTREEITEM>(pNMTreeView->itemNew.hItem);

	Node *node = reinterpret_cast<Node *>(GetItemData(pTreeItem));
	RETURN_IF_FAIL(node);

	enum { EXPAND = 2, SHRINK = 1 };

	switch(pNMTreeView->action)
	{
		case EXPAND:
			node->OnItemExpanded(*this, *node);
			break;
		case SHRINK:
			node->OnItemShrinked(*this, *node);
			break;
	}
}

void Title::OnTvnBeginDrag(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
 
	HTREEITEM pTreeItem = reinterpret_cast<HTREEITEM>(pNMTreeView->itemNew.hItem);

	if(!ItemHasChildren(pTreeItem))
	{
		User *user = reinterpret_cast<User *>(GetItemData(pTreeItem));
		RETURN_IF_FAIL(user != nullptr);

		::SendMessage(AfxGetMainWnd()->m_hWnd, WM_SELECT_USER, 0, (LPARAM)user);
	}
}

void Title::OnRightButtonClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	CPoint point;
    GetCursorPos(&point);

    CPoint pointInTree = point;
    ScreenToClient(&pointInTree);

    HTREEITEM item;
    UINT flag = TVHT_ONITEM;
    item = HitTest(pointInTree, &flag);
    if(item != nullptr)
    {
        SelectItem(item);
		Node *node = reinterpret_cast<Node *>(GetItemData(item));
		if(node != nullptr)
		{
			CString menu_str;
			switch(node->node_type_)
			{
				case TITLE_NODE:
					menu_str.Format(L"查看大区");
					break;
				case TITLE_ROOM:
					menu_str.Format(L"查看房间");
					break;
				default:
					return;
			}

			CMenu menu;
			if(menu.CreatePopupMenu())
			{
				menu.AppendMenu(MF_STRING, IDC_MENU_LOOKUP, menu_str);
				menu.TrackPopupMenu(TPM_LEFTALIGN, pointInTree.x, pointInTree.y, this);
			}
		}
		*pResult = 0;
    }
}

static Node *old_node = nullptr;
void Title::OnMouseMove(UINT nFlags, CPoint point)
{
	static int oldX, oldY;
    int newX = point.x, newY = point.y;

	if ((newX != oldX) || (newY != oldY))
    {
        oldX = newX;
        oldY = newY;
	}

	HTREEITEM hitem = HitTest(point, &nFlags);
	if(hitem != nullptr)
	{
		if (!g_TrackingMouse)
		{
			TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT)};
			tme.hwndTrack = m_hWnd;
			tme.dwFlags = TME_LEAVE;

			TrackMouseEvent(&tme);

			::SendMessage(g_hwndTrackingTT, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&g_toolItem);
 
			g_TrackingMouse = TRUE; 
		}

		Node *new_node = reinterpret_cast<Node *>(GetItemData(hitem));
		if(new_node != nullptr)
		{
			if(new_node != old_node)
			{
				old_node = new_node;
				WCHAR coords[128];
				switch(new_node->node_type_)
				{
					case TITLE_NODE:
						swprintf_s(coords, ARRAYSIZE(coords), _T("大区ID: %d 人数: %u"), new_node->id_, new_node->usercount_);
						break;
					case TITLE_ROOM:
						swprintf_s(coords, ARRAYSIZE(coords), _T("房间ID: %d 人数: %u"), new_node->id_, new_node->usercount_);
						break;
					case TITLE_USER:
						swprintf_s(coords, ARRAYSIZE(coords), _T("音频通道: %u 视频通道: %u 麦序: %u"),
							reinterpret_cast<User *>(new_node)->audio_ssrc_,
							reinterpret_cast<User *>(new_node)->video_ssrc_,
							reinterpret_cast<User *>(new_node)->mic_id_);
						break;
					default:
						break;
				}

				g_toolItem.lpszText = coords;
				::SendMessage(g_hwndTrackingTT, TTM_SETTOOLINFO, 0, (LPARAM)&g_toolItem);

				POINT pt = {newX, newY};
				::ClientToScreen(m_hWnd, &pt);
				::SendMessage(g_hwndTrackingTT, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(pt.x + 10, pt.y - 20));
			}
		}
	}
	else
	{
		::SendMessage(g_hwndTrackingTT, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&g_toolItem);
		g_TrackingMouse = FALSE;
	}

	CTreeCtrl::OnMouseMove(nFlags, point); 
}

void Title::OnMouseLeave()
{
	if(g_TrackingMouse)
	{
		::SendMessage(g_hwndTrackingTT, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&g_toolItem);
		g_TrackingMouse = FALSE;
	}

	CTreeCtrl::OnMouseLeave();
}

void Title::OnLookUpNode(UINT nID)
{
	return;
}
