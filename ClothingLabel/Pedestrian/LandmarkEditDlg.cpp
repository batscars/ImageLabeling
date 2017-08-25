
// LandmarkEditDlg.cpp : implementation file
#include "stdafx.h"
#include "LandmarkEdit.h"
#include "LandmarkEditDlg.h"
#include "afxdialogex.h"
#include "afxwin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define C1 52845
#define C2 22719
#define IM_WIDTH 1200         //固定图片的长宽比例
#define IM_HEIGHT 900

float ratio = 2.439;
int LEFT_BEGIN;
int TOP_BEGIN = 50;

/*画图相关全局变量*/
CString imagePath;            //当前处理图片的绝对路径
CString basePath = L"";       //当前处理图片的上级路径
CString currentImageName;     //当前处理图片的名称
char *PicPath;                //当前处理图片绝对路径的字符串数组表示
CString save_path = L"";      //数据库文件保存的文件夹，以及处理之后图片的存放路径
CString tmpPath =L"";

int image_width = 0;          //图片的宽度
int image_height = 0;         //图片的高度
CRect MainRect;               //客户区
CDC dc_mem;                   //内存画布句柄
CDC *pDC = NULL;
CBitmap bitmap;
CPoint rect_start(-1, -1);    //自定义人体矩形框的左上和右下点
CPoint rect_end(-1, -1);
CPoint move_point(-1, -1);
CPoint resize_lt(-1, -1);
CPoint resize_rt(-1, -1);
CPoint resize_lb(-1, -1);
CPoint resize_rb(-1, -1);
CPoint resize_u(-1, -1);
CPoint resize_l(-1, -1);
CPoint resize_point(-1, -1);
CPoint down(-1, -1);          //左键按下和起来为同一坐标则将以上坐标点都清空
int r_p = -1;                 //记录调整点的编号

int pro_dlg_left;
int pro_dlg_top;

/*数据库相关全局变量*/
sqlite3 *db;
char *dbname;
int existed_image_id;         //如果当前图片数据在数据库中存在，则获取该图片的image_id，不存在则为0
CString op_id;                //当前用户id
int figure_amount;            //当前数据库中figure_id的最大值
int image_amount;             //当前数据库中image_id的最大值
int cur_figure = -1;          //当前操作的figure_id
void* m_pMutex;

/*标志位*/
bool is_move = false;         //鼠标移动标志
bool from_db = false;         //是否从数据库读取数据
bool save_done = true;        //是否将当前处理图片剪切到了指定文件夹
bool move_rect = false;       //人体矩形框移动标志
bool resize_rect = false;     //调整矩形框标志

/*存储不同对话框之间的传递数据*/
std::map<string, string, greater<string>> PropertyValue;  //属性与属性值对应map
std::map<string, std::vector<string>>  pro_values;        //每个属性对应的所有可选值
std::vector<string> FieldName;                            //可编辑字段的字段名
std::map<string, string> operators;                       //用户信息
std::vector<string> to_del_ops;                           //保存准备删除的用户id
std::map<string, COLORREF> color_rgb;
std::map<string, double> color_hvalue;
std::map<string, cv::Scalar> color_scr;
std::vector<int> erased_figure;
std::vector<int> added_figure;

SOCKET sockClient;
//unsigned int __stdcall ThreadFun(PVOID pM)
//{
//	HANDLE hd = CreateFile(tmpPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
//	if (hd != INVALID_HANDLE_VALUE)
//	{
//		CloseHandle(hd);
//		_endthreadex(0);
//	}
//	return 0;
//}

/*CColorComboBox 自定义带颜色下拉框*/
class CColorComboBox : public CComboBox
{
	DECLARE_DYNAMIC(CColorComboBox)

public:
	CColorComboBox();
	virtual ~CColorComboBox();

protected:
	DECLARE_MESSAGE_MAP()

public:
	int AddItem(LPCTSTR lpszText, COLORREF clrValue);
	COLORREF GetCurColor();
	virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
};
IMPLEMENT_DYNAMIC(CColorComboBox, CComboBox)
CColorComboBox::CColorComboBox(){}
CColorComboBox::~CColorComboBox(){}
BEGIN_MESSAGE_MAP(CColorComboBox, CComboBox)
	ON_WM_MEASUREITEM_REFLECT()
END_MESSAGE_MAP()

// CColorComboBox 消息处理程序
int CColorComboBox::AddItem(LPCTSTR lpszText, COLORREF clrValue)
{
	int nIndex = AddString(lpszText);
	SetItemData(nIndex, clrValue);
	return nIndex;
}

COLORREF CColorComboBox::GetCurColor()
{
	int nIndex = GetCurSel();
	if (nIndex != -1)
	{
		return GetItemData(nIndex);
	}
	else
	{
		return -1;
	}
}

void CColorComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	//验证是否为组合框控件
	ASSERT(lpDrawItemStruct->CtlType == ODT_COMBOBOX);
	CDC dc;
	dc.Attach(lpDrawItemStruct->hDC);

	//获取项目区域
	CRect itemRC(lpDrawItemStruct->rcItem);
	//定义显示颜色的区域
	CRect clrRC = itemRC;
	//定义文本区域
	CRect textRC = itemRC;
	//获取系统文本颜色
	COLORREF clrText = GetSysColor(COLOR_WINDOWTEXT);
	//选中时的文本颜色
	COLORREF clrSelected = GetSysColor(COLOR_HIGHLIGHT);
	//获取窗口背景颜色
	COLORREF clrNormal = GetSysColor(COLOR_WINDOW);
	//获取当前项目索引
	int nIndex = lpDrawItemStruct->itemID;
	//判断项目状态
	int nState = lpDrawItemStruct->itemState;
	if (nState & ODS_SELECTED)	//处于选中状态
	{
		dc.SetTextColor((0x00FFFFFF & ~(clrText)));		//文本颜色取反
		dc.SetBkColor(clrSelected);						//设置文本背景颜色
		dc.FillSolidRect(&clrRC, clrSelected);			//填充项目区域为高亮效果
	}
	else
	{
		dc.SetTextColor(clrText);						//设置正常的文本颜色
		dc.SetBkColor(clrNormal);						//设置正常的文本背景颜色
		dc.FillSolidRect(&clrRC, clrNormal);
	}
	if (nState & ODS_FOCUS)								//如果项目获取焦点，绘制焦点区域
	{
		dc.DrawFocusRect(&itemRC);
	}

	//计算文本区域
	int nclrWidth = itemRC.Width() / 4;
	textRC.left = nclrWidth + 55;

	//计算颜色显示区域
	clrRC.DeflateRect(2, 2);
	clrRC.right = nclrWidth + 50;

	//绘制颜色文本并且填充颜色区域
	if (nIndex != -1)	//项目不为空
	{
		//获取项目颜色
		COLORREF clrItem = GetItemData(nIndex);
		dc.SetBkMode(TRANSPARENT);
		//获取文本
		CString szText;
		GetLBText(nIndex, szText);
		//输出文本
		dc.DrawText(szText, textRC, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		dc.FillSolidRect(&clrRC, clrItem);
		//输出颜色
		dc.FrameRect(&clrRC, &CBrush(RGB(0, 0, 0)));
	}
	dc.Detach();
}

void CColorComboBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	//if ((lpMeasureItemStruct->CtlType == ODT_COMBOBOX) &&
	//	(lpMeasureItemStruct->itemID != (UINT)-1))
	//{
	//	CString cstrBuffer;
	//	GetLBText(lpMeasureItemStruct->itemID, cstrBuffer);
	//	CSize sz;
	//	CDC* pDC = GetDC();
	//	sz = pDC->GetTextExtent(cstrBuffer);
	//	ReleaseDC(pDC);
	//	lpMeasureItemStruct->itemHeight = sz.cy * 3 / 2;
	//}

}

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();
// Dialog Data
	enum { IDD = IDD_ABOUTBOX };
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};
CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{}
void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}
BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

/** 
CEditPropertyDlg 编辑人脸属性对话框
**/
class CEditPropertyDlg : public CDialogEx
{
public:
	CRect edit_rect;
	CStatic m_text[10];
	CColorComboBox m_combobox[10];
	int dlg_width = 320;
	int dlg_height = 300;
	CEditPropertyDlg();
	enum {IDD = IDD_EDIT_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
public:
	void InitWidget();
	//afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	//afx_msg void OnSelchangeCombo(UINT nID);
	DECLARE_MESSAGE_MAP()
};

CEditPropertyDlg::CEditPropertyDlg() : CDialogEx(CEditPropertyDlg::IDD)
{}

void CEditPropertyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CEditPropertyDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	dlg_height = 40 * (PropertyValue.size() + 2);
	SetWindowPos(&wndTopMost, pro_dlg_left, pro_dlg_top, dlg_width, dlg_height, NULL);

	GetClientRect(&edit_rect);
	InitWidget();

	CRect ok_rect;
	GetDlgItem(IDOK)->GetWindowRect(&ok_rect);
	ScreenToClient(&ok_rect);
	GetDlgItem(IDOK)->MoveWindow(edit_rect.left + 50, edit_rect.top + 40 * PropertyValue.size(), ok_rect.Width(), ok_rect.Height(), TRUE);

	CRect cancel_rect;
	GetDlgItem(IDCANCEL)->GetWindowRect(&cancel_rect);
	ScreenToClient(&cancel_rect);
	GetDlgItem(IDCANCEL)->MoveWindow(edit_rect.left + 150, edit_rect.top + 40 * PropertyValue.size(), cancel_rect.Width(), cancel_rect.Height(), TRUE);

	return true;
}

void CEditPropertyDlg::OnOK()
{
	int i = 0;
	std::map<string, string, greater<string>>::iterator it;
	for (it = PropertyValue.begin(); it != PropertyValue.end(); ++it)
	{
		CString text(L"");
		m_combobox[i].GetLBText(m_combobox[i].GetCurSel(), text);
		it->second = CStringA(text);
		i++;
	}

	CDialogEx::OnOK();
}

BOOL CEditPropertyDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
		return TRUE;
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
		return TRUE;
	else
		return CDialog::PreTranslateMessage(pMsg);
}

void CEditPropertyDlg::InitWidget()
{
	std::map<string, string, greater<string>>::iterator it;
	int i = 0;
	for (it = PropertyValue.begin(); it != PropertyValue.end(); ++it)
	{
		string caption = it->first;
		CString m_caption = CString(caption.c_str());
		m_text[i].Create(m_caption, WS_CHILD | WS_VISIBLE, CRect(edit_rect.left + 20, edit_rect.top + 5 + i * 40, edit_rect.left + 120, edit_rect.top + i * 40 + 35), 
			this, IDC_MY_TEXT + i);

		m_combobox[i].Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS , CRect(edit_rect.left + 140, edit_rect.top + 5 + i * 40,
			edit_rect.left + 290, edit_rect.top + i * 40 + 35), this, IDC_MY_COMBO + i);
		std::vector<string> values = pro_values[caption];
		if (it->second != "")
		{
			if (values.size() == 9)
			{
				m_combobox[i].AddItem(CString(it->second.c_str()), color_rgb[it->second]);
				for (int j = 0; j < values.size(); ++j)
				{
					if (values[j] != it->second)
					{
						m_combobox[i].AddItem(CString(values[j].c_str()), color_rgb[values[j]]);
					}
				}
				m_combobox[i].SetCurSel(0);
			}
			else
			{
				m_combobox[i].AddString(CString(it->second.c_str()));
				for (int j = 0; j < values.size(); ++j)
				{
					if (values[j] != it->second)
					{
						m_combobox[i].AddString(CString(values[j].c_str()));
					}
				}
				m_combobox[i].SetCurSel(0);
			}
		}
		else
		{
			if (values.size() == 9)
			{
				for (int j = 0; j < values.size(); ++j)
				{
					m_combobox[i].AddItem(CString(values[j].c_str()), color_rgb[values[j]]);
				}
			}
			else
			{
				for (int j = 0; j < values.size(); ++j)
				{
					m_combobox[i].AddString(CString(values[j].c_str()));
				}
			}
		}

		i++;
	}
}

BEGIN_MESSAGE_MAP(CEditPropertyDlg, CDialogEx)
END_MESSAGE_MAP()

/**
CDelOperatorDlg 删除用户对话框
**/
class CDelOperatorDlg : public CDialogEx
{
public:
	CListCtrl m_list;
	CDelOperatorDlg();
	enum { IDD = IDD_DEL_OPERATOR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
public:
	DECLARE_MESSAGE_MAP()
};

CDelOperatorDlg::CDelOperatorDlg() : CDialogEx(CDelOperatorDlg::IDD)
{}

void CDelOperatorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_OP, m_list);
}

BOOL CDelOperatorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	DWORD dwStyle = m_list.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;//选中某行使整行高亮（只适用与report风格的listctrl）
	dwStyle |= LVS_EX_GRIDLINES;//网格线（只适用与report风格的listctrl）
	m_list.SetExtendedStyle(dwStyle); //设置扩展风格

	if (!operators.empty())
	{
		m_list.InsertColumn(0, L"用户ID", LVCFMT_LEFT, 80);
		m_list.InsertColumn(1, L"用户名", LVCFMT_LEFT, 80);
		std::map<string, string>::iterator it;
		int i = 0;
		for (it = operators.begin(); it != operators.end(); ++it, i++)
		{
			USES_CONVERSION;
			m_list.InsertItem(i, NULL);
			m_list.SetItemText(i, 0, A2W((it->first).c_str()));
			m_list.SetItemText(i, 1, A2W((it->second).c_str()));
		}
	}
	return true;
}

void CDelOperatorDlg::OnOK()
{
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	while (pos){
		int nIndex = m_list.GetNextSelectedItem(pos);
		string ops = (CStringA)m_list.GetItemText(nIndex, 0);
		to_del_ops.push_back(ops);
	}
	CDialogEx::OnOK();
}

BOOL CDelOperatorDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
		return TRUE;
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
		return TRUE;
	else
		return CDialog::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CDelOperatorDlg, CDialogEx)
END_MESSAGE_MAP()

/**
CLoginDlg 用户登录对话框
*/
class CLoginDlg : public CDialogEx
{
public:
	CString operator_id;
	CString password;
	CLoginDlg();
	enum{ IDD = IDD_LOGIN};

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
protected:
	DECLARE_MESSAGE_MAP();
};

CLoginDlg::CLoginDlg() : CDialogEx(CLoginDlg::IDD)
{}

BOOL CLoginDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	return true;
}

void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_OPERATOR, operator_id);
	DDX_Text(pDX, IDC_PASSWORD, password);
}

BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
END_MESSAGE_MAP()

/**
CAddOperatorDlg 添加用户对话框
*/
class CAddOperatorDlg : public CDialogEx
{
public:
	CString operator_id;
	CString password;
	CString op_name;
	CAddOperatorDlg();
	enum{ IDD = IDD_ADD_OPERATOR };

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
	DECLARE_MESSAGE_MAP();
};

CAddOperatorDlg::CAddOperatorDlg() : CDialogEx(CAddOperatorDlg::IDD)
{}

BOOL CAddOperatorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	return true;
}

void CAddOperatorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, operator_id);
	DDX_Text(pDX, IDC_EDIT2, password);
	DDX_Text(pDX, IDC_EDIT3, op_name);
}

BOOL CAddOperatorDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
		return TRUE;
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
		return TRUE;
	else
		return CDialog::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CAddOperatorDlg, CDialogEx)
END_MESSAGE_MAP()

/**
CChangePwdDlg 更改用户密码对话框
*/
class CChangePwdDlg : public CDialogEx
{
public:
	CString old_pwd;
	CString new_pwd;
	CChangePwdDlg();
	enum{ IDD = IDD_CHANGE_PWD };

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	DECLARE_MESSAGE_MAP();
};

CChangePwdDlg::CChangePwdDlg() : CDialogEx(CChangePwdDlg::IDD)
{}

BOOL CChangePwdDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	return true;
}

void CChangePwdDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_OLD, old_pwd);
	DDX_Text(pDX, IDC_NEW, new_pwd);
}

BOOL CChangePwdDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
		return TRUE;
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
		return TRUE;
	else
		return CDialog::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CChangePwdDlg, CDialogEx)
END_MESSAGE_MAP()

/**
CAddPropertyDlg 添加属性对话框
*/
class CAddPropertyDlg : public CDialogEx
{
public:
	CString property_name;
	CComboBox m_combobox;
	int selected;
	CString property_value;
	CAddPropertyDlg();
	enum{ IDD = IDD_ADD_PRO };
	afx_msg void OnCbnSelchange();
protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
	DECLARE_MESSAGE_MAP();
};

CAddPropertyDlg::CAddPropertyDlg() : CDialogEx(CAddPropertyDlg::IDD)
{}

BOOL CAddPropertyDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	m_combobox.AddString(L"人物属性");
	m_combobox.AddString(L"衣物属性");
	m_combobox.SetCurSel(0);
	selected = 0;

	return true;
}

void CAddPropertyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PRO_NAME, property_name);
	DDX_Text(pDX, IDC_PRO_VALUE, property_value);
	DDX_Control(pDX, IDC_COMBO, m_combobox);
}

BOOL CAddPropertyDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
		return TRUE;
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
		return TRUE;
	else
		return CDialog::PreTranslateMessage(pMsg);
}

void CAddPropertyDlg::OnCbnSelchange()
{
	selected = m_combobox.GetCurSel();
}

BEGIN_MESSAGE_MAP(CAddPropertyDlg, CDialogEx)
	ON_CBN_SELCHANGE(IDC_COMBO, &CAddPropertyDlg::OnCbnSelchange)
END_MESSAGE_MAP()

class CMessageDlg : public CDialogEx
{
public:
	CMessageDlg();
	enum { IDD = IDD_MESSAGE };
protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnTimer(UINT_PTR nIDEvent);
protected:
	DECLARE_MESSAGE_MAP()
};

CMessageDlg::CMessageDlg() : CDialogEx(CMessageDlg::IDD)
{
}

void CMessageDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMessageDlg, CDialogEx)
	ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CMessageDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetTimer(1, 100, NULL);
	return TRUE;
}

void CMessageDlg::OnTimer(UINT_PTR nIDEvent)
{
	CDialogEx::OnTimer(nIDEvent);
	EndDialog(0);
}

/**
CLandmarkEditDlg 主对话框
**/
CLandmarkEditDlg::CLandmarkEditDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CLandmarkEditDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CLandmarkEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_PATH, m_listCtrl);
	DDX_Control(pDX, IDC_DATETIMEPICKER1, m_dateStart);
	DDX_Control(pDX, IDC_DATETIMEPICKER2, m_dateEnd);
}

BEGIN_MESSAGE_MAP(CLandmarkEditDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_QUERYDRAGICON()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_ERASEBKGND()
	ON_WM_CLOSE()
	ON_COMMAND(ID_OPEN_PIC, &CLandmarkEditDlg::OnOpenPic)
	ON_COMMAND(ID_EDIT_PRO, &CLandmarkEditDlg::OnEditPro)
	ON_BN_CLICKED(IDC_NEXT_PIC, &CLandmarkEditDlg::OnBnClickedNextPic)
	ON_BN_CLICKED(IDC_SEARCH, &CLandmarkEditDlg::OnBnClickedSearch)
	ON_NOTIFY(NM_CLICK, IDC_LIST_PATH, &CLandmarkEditDlg::OnNMClickList)
	ON_BN_CLICKED(IDC_SAVE, &CLandmarkEditDlg::OnBnClickedSave)
	ON_COMMAND(ID_ADD_OPERATOR, &CLandmarkEditDlg::OnAddOperator)
	ON_COMMAND(ID_EXIT, &CLandmarkEditDlg::OnExit)
	ON_COMMAND(ID_CHANGE_PWD, &CLandmarkEditDlg::OnChangePwd)
	ON_COMMAND(ID_ADD_PRO, &CLandmarkEditDlg::OnAddPro)
	ON_BN_CLICKED(IDC_DEL_PIC, &CLandmarkEditDlg::OnBnClickedDelPic)
	ON_COMMAND(ID_DEL_OPERATOR, &CLandmarkEditDlg::OnDelOperator)
	ON_COMMAND(ID_BODY_RECT, &CLandmarkEditDlg::OnBodyRect)
	ON_COMMAND(ID_UPPER_RECT, &CLandmarkEditDlg::OnUpperRect)
	ON_COMMAND(ID_DELETE_FIGURE, &CLandmarkEditDlg::OnDeleteFigure)
	ON_COMMAND(ID_MOV_RECT, &CLandmarkEditDlg::OnMovRect)
	ON_COMMAND(ID_CANCEL_RECT, &CLandmarkEditDlg::OnCancelRect)
END_MESSAGE_MAP()

//CLandmarkEditDlg message handlers
BOOL CLandmarkEditDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	save_path = SetSavePath();
	if (save_path.IsEmpty())
	{
		EndDialog(0);
		return true;
	}
	CString db_path = save_path + "\\feature_v2.sqlite";

	char *tmp_db;
	int len = WideCharToMultiByte(CP_ACP, 0, db_path, -1, NULL, 0, NULL, NULL);
	tmp_db = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, db_path, -1, tmp_db, len, NULL, NULL);

	dbname = unicodeToUtf8(mbcsToUnicode(tmp_db));

	bool op_ex = DB_OperatorExisted();
	if (op_ex)
	{
		CLoginDlg login;
		INT_PTR res = login.DoModal();
		if (IDOK == res)
		{
			UpdateData(true);
			if (DB_AllowLogin(login.operator_id, login.password))
			{
				op_id = login.operator_id;
				DB_ChangeState(op_id, L"login");
			}
			else
			{
				AfxMessageBox(L"密码错误!！", MB_OK | MB_ICONERROR);
				EndDialog(0);
				return true;
			}
		}
		else
		{
			EndDialog(0);
			return true;
		}
	}
	else
	{
		CAddOperatorDlg add;
		INT_PTR res = add.DoModal();
		if (IDOK == res)
		{
			UpdateData(true);
			if (!add.operator_id.IsEmpty() && !add.password.IsEmpty())
			{
				DB_InsertOperator(add.operator_id, add.op_name, add.password, L"admin", L"login");
				op_id = add.operator_id;
			}
			else
			{
				EndDialog(0);
				return true;
			}
		}
		else
		{
			EndDialog(0);
			return true;
		}
	}

	ShowWindow(SW_SHOWMAXIMIZED);

	SetWindowText(L"行人衣物标注工具");

	// IDM_ABOUTBOX must be in the system command range.
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

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	GetClientRect(&MainRect);

	LEFT_BEGIN = MainRect.Width() / 5;

	pDC = GetDC();
	
	//FieldName = GetFigureFieldName(DB_GetFigureCreateSql());
    InitProValues();
	color_rgb = InitColorRGB();
	color_scr = InitColorScalar();

	SetDlgItemText(IDC_TEXT01, L"当前用户ID为：" + op_id);

	CRect save_rect;
	GetDlgItem(IDC_SAVE)->GetWindowRect(&save_rect);
	ScreenToClient(&save_rect);
	GetDlgItem(IDC_SAVE)->MoveWindow(LEFT_BEGIN + 10, 950, save_rect.Width(), save_rect.Height(), TRUE);

	CRect rgb_rect;
	GetDlgItem(IDC_RGB_V)->GetWindowRect(&rgb_rect);
	ScreenToClient(&rgb_rect);
	GetDlgItem(IDC_RGB_V)->MoveWindow(LEFT_BEGIN + 10, 10, rgb_rect.Width(), rgb_rect.Height(), TRUE);

	CRect image_rect;
	GetDlgItem(IDC_IMAGE_NAME)->GetWindowRect(&image_rect);
	ScreenToClient(&image_rect);
	GetDlgItem(IDC_IMAGE_NAME)->MoveWindow(rgb_rect.right + 150, 10, image_rect.Width(), image_rect.Height(), TRUE);

	CRect next_rect;
	GetDlgItem(IDC_NEXT_PIC)->GetWindowRect(&next_rect);
	ScreenToClient(&next_rect);
	GetDlgItem(IDC_NEXT_PIC)->MoveWindow(LEFT_BEGIN + 100, 950, next_rect.Width(), next_rect.Height(), TRUE);

	CRect del_rect;
	GetDlgItem(IDC_DEL_PIC)->GetWindowRect(&del_rect);
	ScreenToClient(&del_rect);
	GetDlgItem(IDC_DEL_PIC)->MoveWindow(LEFT_BEGIN + 190, 950, del_rect.Width(), del_rect.Height(), TRUE);

	return TRUE;
}

BOOL CLandmarkEditDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
		return TRUE;
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
		return TRUE;
	else
		return CDialog::PreTranslateMessage(pMsg);
}

BOOL CLandmarkEditDlg::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CLandmarkEditDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CLandmarkEditDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();

		CClientDC dc(this);
		CPen pen(PS_SOLID, 1, RGB(0, 0, 0));
		CPen *pOldPen = dc.SelectObject(&pen);
		dc.MoveTo(CPoint(MainRect.Width() / 5 - 5, 0));
		dc.LineTo(CPoint(MainRect.Width() / 5 - 5, MainRect.Height()));
		dc.SelectObject(pOldPen);
        
		DrawOnBuffer();

	}
}

HCURSOR CLandmarkEditDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CLandmarkEditDlg::ChangeSize(UINT nID, int x, int y)
{
	CWnd *pWnd;
	pWnd = GetDlgItem(nID);
	if (pWnd != NULL)
	{
		CRect rec;
		pWnd->GetWindowRect(&rec);
		ScreenToClient(&rec);
		rec.left = rec.left*x / MainRect.Width();
		rec.top = rec.top*y / MainRect.Height();
		rec.bottom = rec.bottom*y / MainRect.Height();
		rec.right = rec.right*x / MainRect.Width();
		pWnd->ShowWindow(FALSE);
		pWnd->ShowWindow(TRUE);
		pWnd->MoveWindow(rec);
	}
}

void CLandmarkEditDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if (nType != SIZE_MINIMIZED)
	{
		GetClientRect(&MainRect);
	}
}

void CLandmarkEditDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (point.x >= move_point.x - 4 && point.x <= move_point.x + 4 && point.y >= move_point.y - 4 && point.y <= move_point.y + 4)
	{
		move_rect = true;
		resize_rect = false;
		is_move = true;
	}
	else if (point.x >= resize_u.x - 4 && point.x <= resize_u.x + 4 && point.y >= resize_u.y - 4 && point.y <= resize_u.y + 4)
	{
		resize_rect = true;
		move_rect = false;
		is_move = true;
		r_p = 5;
	}
	else if (point.x >= resize_l.x - 4 && point.x <= resize_l.x + 4 && point.y >= resize_l.y - 4 && point.y <= resize_l.y + 4)
	{
		resize_rect = true;
		is_move = true;
		move_rect = false;
		r_p = 6;
	}
	else if (point.x >= resize_lt.x - 4 && point.x <= resize_lt.x + 4 && point.y >= resize_lt.y - 4 && point.y <= resize_lt.y + 4)
	{
		resize_rect = true;
		is_move = true;
		move_rect = false;
		r_p = 1;
	}
	else if (point.x >= resize_rb.x - 4 && point.x <= resize_rb.x + 4 && point.y >= resize_rb.y - 4 && point.y <= resize_rb.y + 4)
	{
		resize_rect = true;
		is_move = true;
		move_rect = false;
		r_p = 2;
	}
	else if (point.x >= resize_rt.x - 4 && point.x <= resize_rt.x + 4 && point.y >= resize_rt.y - 4 && point.y <= resize_rt.y + 4)
	{
		resize_rect = true;
		is_move = true;
		move_rect = false;
		r_p = 3;
	}
	else if (point.x >= resize_lb.x - 4 && point.x <= resize_lb.x + 4 && point.y >= resize_lb.y - 4 && point.y <= resize_lb.y + 4)
	{
		resize_rect = true;
		is_move = true;
		move_rect = false;
		r_p = 4;
	}
	else if (!move_rect && !resize_rect && point.x >= LEFT_BEGIN && point.x <= LEFT_BEGIN + IM_WIDTH && point.y >= TOP_BEGIN && point.y <= TOP_BEGIN + IM_HEIGHT)
	{
		is_move = true;
		rect_start = point;
	}
	down = point;
	CDialogEx::OnLButtonDown(nFlags, point);
}

void CLandmarkEditDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (down == point)
	{
		rect_start = (-1, -1);
		rect_end = (-1, -1);
		move_point = (-1, -1);
		resize_rect = false;
		move_rect = false;
		resize_l = (-1, -1);
		resize_u = (-1, -1);
		cur_figure = -1;
		Invalidate();
	}
	else if (is_move && !move_rect  && !resize_rect && point.x >= LEFT_BEGIN && point.x <= LEFT_BEGIN + IM_WIDTH && point.y >= TOP_BEGIN && point.y <= TOP_BEGIN + IM_HEIGHT && rect_start != point)
	{
		rect_end.y = point.y;
		rect_end.x = ((point.y - rect_start.y) * image_height / (ratio*IM_HEIGHT) + rect_start.x * image_width / IM_WIDTH)*IM_WIDTH / image_width;
		is_move = false;

		move_point.y = rect_start.y;
		move_point.x = (rect_start.x + rect_end.x) / 2;

		resize_lt = rect_start;
		resize_rb = rect_end;
		resize_lb = CPoint(rect_start.x, rect_end.y);
		resize_rt = CPoint(rect_end.x, rect_start.y);

		Invalidate();
	}
	else if (is_move && move_rect && point.x >= LEFT_BEGIN && point.x <= LEFT_BEGIN + IM_WIDTH && point.y >= TOP_BEGIN && point.y <= TOP_BEGIN + IM_HEIGHT)
	{
		int cx = point.x - move_point.x;
		int cy = point.y - move_point.y;
		if (cur_figure == -1)
		{
			rect_start.x += cx;
			rect_start.y += cy;
			rect_end.x += cx;
			rect_end.y += cy;

			move_point = point;	
			resize_lt = rect_start;
			resize_rb = rect_end;
			resize_lb = CPoint(rect_start.x, rect_end.y);
			resize_rt = CPoint(rect_end.x, rect_start.y);
		}
		else
		{
			int dx = cx * image_width / IM_WIDTH;
			int dy = cy * image_height / IM_HEIGHT;
			CRect b_rect = db_rect[cur_figure];
			db_rect[cur_figure] = CRect(b_rect.left + dx, b_rect.top + dy, b_rect.right + dx, b_rect.bottom + dy);
			//DB_UpdateBodyRect(db_rect[cur_figure], cur_figure);

			if (db_upper_rect.find(cur_figure) != db_upper_rect.end())
			{
				CRect u_rect = db_upper_rect[cur_figure];
				db_upper_rect[cur_figure] = CRect(u_rect.left + dx, u_rect.top + dy, u_rect.right + dx, u_rect.bottom + dy);
				//DB_InsertUpperRect(db_upper_rect[cur_figure].left, db_upper_rect[cur_figure].top, 
					             //  db_upper_rect[cur_figure].Width(), db_upper_rect[cur_figure].Height(), cur_figure);
				
				CRect l_rect = db_lower_rect[cur_figure];
				db_lower_rect[cur_figure] = CRect(l_rect.left + dx, l_rect.top + dy, l_rect.right + dx, l_rect.bottom + dy);
				//DB_InsertLowerRect(db_lower_rect[cur_figure].left, db_lower_rect[cur_figure].top,
					  //db_lower_rect[cur_figure].Width(), db_lower_rect[cur_figure].Height(), cur_figure);

			}
			int l = (db_rect[cur_figure].left + db_rect[cur_figure].right) / 2;
			move_point = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
			resize_lt = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
			resize_rt = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
			resize_lb = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
			resize_rb = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
			resize_u = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].top * IM_HEIGHT / image_height);
			resize_l = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].bottom * IM_HEIGHT / image_height);
		}
		is_move = false;
		Invalidate();
	}
	else if (is_move && resize_rect && point.x >= LEFT_BEGIN && point.x <= LEFT_BEGIN + IM_WIDTH && point.y >= TOP_BEGIN && point.y <= TOP_BEGIN + IM_HEIGHT)
	{
		if (r_p == 1)
		{
			if (cur_figure == -1)
			{
				rect_start.y = point.y;
				rect_start.x = (rect_end.x * image_width / IM_WIDTH - (rect_end.y - point.y) * image_height / (ratio*IM_HEIGHT))*IM_WIDTH / image_width;

				resize_lt = rect_start;
				resize_rt = CPoint(rect_end.x, rect_start.y);
				resize_lb = CPoint(rect_start.x, rect_end.y);

				move_point.y = rect_start.y;
				move_point.x = (rect_start.x + rect_end.x) / 2;
			}
			else
			{
				int x = (resize_rb.x - LEFT_BEGIN)*image_width / IM_WIDTH - (resize_rb.y - point.y) * image_height / (ratio*IM_HEIGHT);
				int y = (point.y - TOP_BEGIN) * image_height / IM_HEIGHT;

				CRect rect = db_rect[cur_figure];
				db_rect[cur_figure] = CRect(x, y, rect.right, rect.bottom);
				//DB_UpdateBodyRect(db_rect[cur_figure], cur_figure);

				if (db_upper_rect.find(cur_figure) != db_upper_rect.end())
				{
					db_upper_rect[cur_figure].left = x;
					db_lower_rect[cur_figure].left = x;

					//DB_InsertUpperRect(db_upper_rect[cur_figure].left, db_upper_rect[cur_figure].top,
						//db_upper_rect[cur_figure].Width(), db_upper_rect[cur_figure].Height(), cur_figure);

					//DB_InsertLowerRect(db_lower_rect[cur_figure].left, db_lower_rect[cur_figure].top,
						//db_lower_rect[cur_figure].Width(), db_lower_rect[cur_figure].Height(), cur_figure);
				}

				int l = (db_rect[cur_figure].left + db_rect[cur_figure].right) / 2;
				move_point = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_lt = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_rt = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_lb = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
				resize_rb = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
				resize_u = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_l = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].bottom * IM_HEIGHT / image_height);
			}
		}
		else if (r_p == 2)
		{
			if (cur_figure == -1)
			{
				rect_end.y = point.y;
				rect_end.x = (rect_start.x * image_width / IM_WIDTH + (point.y - rect_start.y) * image_height / (ratio*IM_HEIGHT))*IM_WIDTH / image_width;

				resize_rb = rect_end;
				resize_rt = CPoint(rect_end.x, rect_start.y);
				resize_lb = CPoint(rect_start.x, rect_end.y);

				move_point.y = rect_start.y;
				move_point.x = (rect_start.x + rect_end.x) / 2;
			}
			else
			{
				int x = (resize_lt.x - LEFT_BEGIN)*image_width / IM_WIDTH + (point.y - resize_lt.y) * image_height / (ratio*IM_HEIGHT);
				int y = (point.y - TOP_BEGIN) * image_height / IM_HEIGHT;

				CRect rect = db_rect[cur_figure];
				db_rect[cur_figure] = CRect(rect.left, rect.top, x, y);
				//DB_UpdateBodyRect(db_rect[cur_figure], cur_figure);

				if (db_upper_rect.find(cur_figure) != db_upper_rect.end())
				{
					db_upper_rect[cur_figure].right = x;
					db_lower_rect[cur_figure].right = x;
					db_lower_rect[cur_figure].bottom = y;

					//DB_InsertUpperRect(db_upper_rect[cur_figure].left, db_upper_rect[cur_figure].top,
						//db_upper_rect[cur_figure].Width(), db_upper_rect[cur_figure].Height(), cur_figure);

					//DB_InsertLowerRect(db_lower_rect[cur_figure].left, db_lower_rect[cur_figure].top,
						//db_lower_rect[cur_figure].Width(), db_lower_rect[cur_figure].Height(), cur_figure);
				}

				int l = (db_rect[cur_figure].left + db_rect[cur_figure].right) / 2;
				move_point = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_lt = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_rt = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_lb = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
				resize_rb = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
				resize_u = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_l = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].bottom * IM_HEIGHT / image_height);

			}
		}
		else if (r_p==3)
		{
			if (cur_figure == -1)
			{
				rect_start.y = point.y;
				rect_end.x = (rect_start.x * image_width / IM_WIDTH + (rect_end.y - point.y) * image_height / (ratio*IM_HEIGHT))*IM_WIDTH / image_width;

				resize_lt = rect_start;
				resize_rb = rect_end;
				resize_rt = CPoint(rect_end.x, rect_start.y);
				resize_lb = CPoint(rect_start.x, rect_end.y);
				move_point.y = rect_start.y;
				move_point.x = (rect_start.x + rect_end.x) / 2;
			}
			else
			{
				int x = (resize_lt.x - LEFT_BEGIN)*image_width / IM_WIDTH + (resize_rb.y - point.y) * image_height / (ratio*IM_HEIGHT);
				int y = (point.y - TOP_BEGIN) * image_height / IM_HEIGHT;

				CRect rect = db_rect[cur_figure];
				db_rect[cur_figure] = CRect(rect.left, y, x, rect.bottom);
				//DB_UpdateBodyRect(db_rect[cur_figure], cur_figure);
				
				if (db_upper_rect.find(cur_figure) != db_upper_rect.end())
				{
					db_upper_rect[cur_figure].right = x;
					db_lower_rect[cur_figure].right = x;

					//DB_InsertUpperRect(db_upper_rect[cur_figure].left, db_upper_rect[cur_figure].top,
						//db_upper_rect[cur_figure].Width(), db_upper_rect[cur_figure].Height(), cur_figure);

					//DB_InsertLowerRect(db_lower_rect[cur_figure].left, db_lower_rect[cur_figure].top,
						//db_lower_rect[cur_figure].Width(), db_lower_rect[cur_figure].Height(), cur_figure);
				}

				int l = (db_rect[cur_figure].left + db_rect[cur_figure].right) / 2;
				move_point = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_lt = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_rt = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_lb = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
				resize_rb = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
				resize_u = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_l = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].bottom * IM_HEIGHT / image_height);
			}
		}
		else if (r_p == 4)
		{
			if (cur_figure == -1)
			{
				rect_end.y = point.y;
				rect_start.x = (rect_end.x * image_width / IM_WIDTH - (point.y - rect_start.y) * image_height / (ratio*IM_HEIGHT))*IM_WIDTH / image_width;

				resize_lt = rect_start;
				resize_rb = rect_end;
				resize_rt = CPoint(rect_end.x, rect_start.y);
				resize_lb = CPoint(rect_start.x, rect_end.y);
				move_point.y = rect_start.y;
				move_point.x = (rect_start.x + rect_end.x) / 2;
			}
			else
			{
				int x = (resize_rb.x - LEFT_BEGIN)*image_width / IM_WIDTH - (point.y - resize_lt.y) * image_height / (ratio*IM_HEIGHT);
				int y = (point.y - TOP_BEGIN) * image_height / IM_HEIGHT;

				CRect rect = db_rect[cur_figure];
				db_rect[cur_figure] = CRect(x, rect.top, rect.right, y);
				//DB_UpdateBodyRect(db_rect[cur_figure], cur_figure);

				if (db_upper_rect.find(cur_figure) != db_upper_rect.end())
				{
					db_upper_rect[cur_figure].left = x;
					db_lower_rect[cur_figure].left = x;
					db_lower_rect[cur_figure].bottom = y;

					//DB_InsertUpperRect(db_upper_rect[cur_figure].left, db_upper_rect[cur_figure].top,
						//db_upper_rect[cur_figure].Width(), db_upper_rect[cur_figure].Height(), cur_figure);

					//DB_InsertLowerRect(db_lower_rect[cur_figure].left, db_lower_rect[cur_figure].top,
						//db_lower_rect[cur_figure].Width(), db_lower_rect[cur_figure].Height(), cur_figure);
				}

				int l = (db_rect[cur_figure].left + db_rect[cur_figure].right) / 2;
				move_point = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_lt = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_rt = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_lb = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
				resize_rb = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
				resize_u = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_l = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].bottom * IM_HEIGHT / image_height);
			}
		}
		else if (r_p == 5)
		{
			int cy = (point.y - resize_u.y) * image_height / IM_HEIGHT;
			CRect u_rect = db_upper_rect[cur_figure];
			db_upper_rect[cur_figure] = CRect(u_rect.left, u_rect.top + cy, u_rect.right, u_rect.bottom);
			resize_u.y = db_upper_rect[cur_figure].top * IM_HEIGHT / image_height + TOP_BEGIN;
			//DB_InsertUpperRect(db_upper_rect[cur_figure].left, db_upper_rect[cur_figure].top,
				//db_upper_rect[cur_figure].Width(), db_upper_rect[cur_figure].Height(), cur_figure);	
		}
		else if (r_p == 6)
		{
			int cy = (point.y - resize_l.y) * image_height / IM_HEIGHT;
			CRect u_rect = db_upper_rect[cur_figure];
			db_upper_rect[cur_figure] = CRect(u_rect.left, u_rect.top, u_rect.right, u_rect.bottom + cy);
			//DB_InsertUpperRect(db_upper_rect[cur_figure].left, db_upper_rect[cur_figure].top,
				//db_upper_rect[cur_figure].Width(), db_upper_rect[cur_figure].Height(), cur_figure);
			
			db_lower_rect[cur_figure].top = db_upper_rect[cur_figure].bottom;
			
			resize_l.y = db_lower_rect[cur_figure].top * IM_HEIGHT / image_height + TOP_BEGIN;
			//DB_InsertLowerRect(db_lower_rect[cur_figure].left, db_lower_rect[cur_figure].top,
				//db_lower_rect[cur_figure].Width(), db_lower_rect[cur_figure].Height(), cur_figure);
		}
		is_move = false;
		Invalidate(); 
	}
	CDialogEx::OnLButtonUp(nFlags, point);
}

void CLandmarkEditDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (is_move && !move_rect && !resize_rect && MK_LBUTTON == nFlags && point.x >= LEFT_BEGIN && point.x <= LEFT_BEGIN + IM_WIDTH && point.y >= TOP_BEGIN && point.y <= TOP_BEGIN + IM_HEIGHT)
	{
		rect_end.y = point.y;
		rect_end.x = ((point.y - rect_start.y) * image_height / (ratio*IM_HEIGHT) + rect_start.x * image_width / IM_WIDTH)*IM_WIDTH / image_width;
		Invalidate();
	}
	else if (is_move && move_rect && !resize_rect && MK_LBUTTON == nFlags && point.x >= LEFT_BEGIN && point.x <= LEFT_BEGIN + IM_WIDTH && point.y >= TOP_BEGIN && point.y <= TOP_BEGIN + IM_HEIGHT)
	{
		int cx = point.x - move_point.x;
		int cy = point.y - move_point.y;
		if (cur_figure == -1)
		{
			rect_start.x += cx;
			rect_start.y += cy;
			rect_end.x += cx;
			rect_end.y += cy;

			move_point = point;
			resize_lt = rect_start;
			resize_rb = rect_end;
			resize_lb = CPoint(rect_start.x, rect_end.y);
			resize_rt = CPoint(rect_end.x, rect_start.y);
		}
		else
		{
			int dx = cx * image_width / IM_WIDTH;
			int dy = cy * image_height / IM_HEIGHT;
			CRect b_rect = db_rect[cur_figure];
			db_rect[cur_figure] = CRect(b_rect.left + dx, b_rect.top + dy, b_rect.right + dx, b_rect.bottom + dy);
			if (db_upper_rect.find(cur_figure)!=db_upper_rect.end())
			{
				CRect u_rect = db_upper_rect[cur_figure];
				db_upper_rect[cur_figure] = CRect(u_rect.left + dx, u_rect.top + dy, u_rect.right + dx, u_rect.bottom + dy);
				
				CRect l_rect = db_lower_rect[cur_figure];
				db_lower_rect[cur_figure] = CRect(l_rect.left + dx, l_rect.top + dy, l_rect.right + dx, l_rect.bottom + dy);
			}

			int l = (db_rect[cur_figure].left + db_rect[cur_figure].right) / 2;
			move_point = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
			resize_lt = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
			resize_rt = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
			resize_lb = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
			resize_rb = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
			resize_u = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].top * IM_HEIGHT / image_height);
			resize_l = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].bottom * IM_HEIGHT / image_height);
		}
		Invalidate();
	}
	else if (is_move && resize_rect && point.x >= LEFT_BEGIN && point.x <= LEFT_BEGIN + IM_WIDTH && point.y >= TOP_BEGIN && point.y <= TOP_BEGIN + IM_HEIGHT)
	{
		if (r_p == 1)
		{
			if (cur_figure == -1)
			{
				rect_start.y = point.y;
				rect_start.x = (rect_end.x * image_width / IM_WIDTH - (rect_end.y - point.y) * image_height / (ratio*IM_HEIGHT))*IM_WIDTH / image_width;

				resize_lt = rect_start;
				resize_rt = CPoint(rect_end.x, rect_start.y);
				resize_lb = CPoint(rect_start.x, rect_end.y);

				move_point.y = rect_start.y;
				move_point.x = (rect_start.x + rect_end.x) / 2;
			}
			else
			{
				int x = (resize_rb.x - LEFT_BEGIN)*image_width / IM_WIDTH - (resize_rb.y - point.y) * image_height / (ratio*IM_HEIGHT);
				int y = (point.y - TOP_BEGIN) * image_height / IM_HEIGHT;

				CRect rect = db_rect[cur_figure];
				db_rect[cur_figure] = CRect(x, y, rect.right, rect.bottom);

				if (db_upper_rect.find(cur_figure) != db_upper_rect.end())
				{
					db_upper_rect[cur_figure].left = x;
					db_lower_rect[cur_figure].left = x;
				}
				
				int l = (db_rect[cur_figure].left + db_rect[cur_figure].right) / 2;
				move_point = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_lt = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_rt = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_lb = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
				resize_rb = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
				resize_u = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_l = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].bottom * IM_HEIGHT / image_height);

			}
		}
		else if (r_p == 2)
		{
			if (cur_figure == -1)
			{
				rect_end.y = point.y;
				rect_end.x = (rect_start.x * image_width / IM_WIDTH + (point.y - rect_start.y) * image_height / (ratio*IM_HEIGHT))*IM_WIDTH / image_width;

				resize_rb = rect_end;
				resize_rt = CPoint(rect_end.x, rect_start.y);
				resize_lb = CPoint(rect_start.x, rect_end.y);

				move_point.y = rect_start.y;
				move_point.x = (rect_start.x + rect_end.x) / 2;
			}
			else
			{
				int x = (resize_lt.x - LEFT_BEGIN)*image_width / IM_WIDTH + (point.y - resize_lt.y) * image_height / (ratio*IM_HEIGHT);
				int y = (point.y - TOP_BEGIN) * image_height / IM_HEIGHT;

				CRect rect = db_rect[cur_figure];
				db_rect[cur_figure] = CRect(rect.left, rect.top, x, y);

				if (db_upper_rect.find(cur_figure) != db_upper_rect.end())
				{
					db_upper_rect[cur_figure].right = x;
					db_lower_rect[cur_figure].right = x;
					db_lower_rect[cur_figure].bottom = y;
				}

				int l = (db_rect[cur_figure].left + db_rect[cur_figure].right) / 2;
				move_point = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_lt = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_rt = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_lb = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
				resize_rb = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
				resize_u = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_l = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].bottom * IM_HEIGHT / image_height);
			}
		}
		else if (r_p == 3)
		{
			if (cur_figure == -1)
			{
				rect_start.y = point.y;
				rect_end.x = (rect_start.x * image_width / IM_WIDTH + (rect_end.y - point.y) * image_height / (ratio*IM_HEIGHT))*IM_WIDTH / image_width;
				
				resize_lt = rect_start;
				resize_rb = rect_end;
				resize_rt = CPoint(rect_end.x, rect_start.y);
				resize_lb = CPoint(rect_start.x, rect_end.y);
				move_point.y = rect_start.y;
				move_point.x = (rect_start.x + rect_end.x) / 2;
			}
			else
			{
				int x = (resize_lt.x - LEFT_BEGIN)*image_width / IM_WIDTH + (resize_rb.y - point.y) * image_height / (ratio*IM_HEIGHT);
				int y = (point.y - TOP_BEGIN) * image_height / IM_HEIGHT;

				CRect rect = db_rect[cur_figure];
				db_rect[cur_figure] = CRect(rect.left, y, x, rect.bottom);

				if (db_upper_rect.find(cur_figure) != db_upper_rect.end())
				{
					db_upper_rect[cur_figure].right = x;
					db_lower_rect[cur_figure].right = x;
				}

				int l = (db_rect[cur_figure].left + db_rect[cur_figure].right) / 2;
				move_point = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_lt = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_rt = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_lb = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
				resize_rb = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
				resize_u = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_l = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].bottom * IM_HEIGHT / image_height);
			}
		}
		else if (r_p == 4)
		{
			if (cur_figure == -1)
			{
				rect_end.y = point.y;
				rect_start.x = (rect_end.x * image_width / IM_WIDTH - (point.y - rect_start.y) * image_height / (ratio*IM_HEIGHT))*IM_WIDTH / image_width;

				resize_lt = rect_start;
				resize_rb = rect_end;
				resize_rt = CPoint(rect_end.x, rect_start.y);
				resize_lb = CPoint(rect_start.x, rect_end.y);
				move_point.y = rect_start.y;
				move_point.x = (rect_start.x + rect_end.x) / 2;
			}
			else
			{
				int x = (resize_rb.x - LEFT_BEGIN)*image_width / IM_WIDTH - (point.y - resize_lt.y) * image_height / (ratio*IM_HEIGHT);
				int y = (point.y - TOP_BEGIN) * image_height / IM_HEIGHT;

				CRect rect = db_rect[cur_figure];
				db_rect[cur_figure] = CRect(x, rect.top, rect.right, y);

				if (db_upper_rect.find(cur_figure) != db_upper_rect.end())
				{
					db_upper_rect[cur_figure].left = x;
					db_lower_rect[cur_figure].left = x;
					db_lower_rect[cur_figure].bottom = y;
				}

				int l = (db_rect[cur_figure].left + db_rect[cur_figure].right) / 2;
				move_point = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_lt = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_rt = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_lb = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
				resize_rb = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
				resize_u = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].top * IM_HEIGHT / image_height);
				resize_l = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].bottom * IM_HEIGHT / image_height);
			}
		}
		else if (r_p == 5)
		{
			int cy = (point.y - resize_u.y) * image_height / IM_HEIGHT;
			CRect u_rect = db_upper_rect[cur_figure];
			db_upper_rect[cur_figure] = CRect(u_rect.left, u_rect.top + cy, u_rect.right, u_rect.bottom);
			resize_u.y = db_upper_rect[cur_figure].top * IM_HEIGHT / image_height + TOP_BEGIN;
		}
		else if (r_p == 6)
		{
			int cy = (point.y - resize_l.y) * image_height / IM_HEIGHT;
			CRect u_rect = db_upper_rect[cur_figure];
			db_upper_rect[cur_figure] = CRect(u_rect.left, u_rect.top, u_rect.right, u_rect.bottom + cy);
			db_lower_rect[cur_figure].top = db_upper_rect[cur_figure].bottom;
			resize_l.y = db_lower_rect[cur_figure].top * IM_HEIGHT / image_height + TOP_BEGIN;
			resize_u.y = db_upper_rect[cur_figure].top * IM_HEIGHT / image_height + TOP_BEGIN;
		}
		Invalidate();
	}
	else
	{
		if (GetCurFigureId(point)!=0)
		{
			CString text = GetRgbValue(point);
			GetDlgItem(IDC_RGB_V)->SetWindowTextW(text);
		}
	}
	CDialogEx::OnMouseMove(nFlags, point);
}

void CLandmarkEditDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (point.x >= rect_start.x && point.x <= rect_end.x && point.y >= rect_start.y && point.y <= rect_end.y)
	{
		CMenu rmenu;
		rmenu.LoadMenuW(IDR_RMENU);
		CMenu *popUp = rmenu.GetSubMenu(0);
		ClientToScreen(&point);
		popUp->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
	}
	else
	{
		int figure = GetCurFigureId(point);
		if (figure != 0)
		{
			cur_figure = figure;

			pro_dlg_left = point.x;
			pro_dlg_top = point.y;

			CMenu rmenu;
			rmenu.LoadMenuW(IDR_EDIT_MENU);
			CMenu *popUp = rmenu.GetSubMenu(0);
			ClientToScreen(&point);
			popUp->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
		}
	}
	
	CDialogEx::OnRButtonDown(nFlags, point);
}

void CLandmarkEditDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	CDialogEx::OnRButtonUp(nFlags, point);
}

void CLandmarkEditDlg::OnOpenPic()
{
	CFileDialog selectPic(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, L"jpg(*.jpg)|*.jpg|png(*.png)|*.png||");
	if (IDOK == selectPic.DoModal())
	{
		CRect rect(LEFT_BEGIN, TOP_BEGIN, LEFT_BEGIN + IM_WIDTH, TOP_BEGIN + IM_HEIGHT);
		pDC->FillSolidRect(&rect, pDC->GetBkColor());

		imagePath = selectPic.GetPathName();
		basePath = selectPic.GetFolderPath();
		currentImageName = selectPic.GetFileName();
		Images = GetRemainingImages(basePath);

		SetDlgItemText(IDC_IMAGE_NAME, currentImageName);

		int len = WideCharToMultiByte(CP_ACP, 0, imagePath, -1, NULL, 0, NULL, NULL);
		PicPath = new char[len + 1];
		WideCharToMultiByte(CP_ACP, 0, imagePath, -1, PicPath, len, NULL, NULL);

		if (!db_rect.empty())
		{
			db_rect.clear();
		}
		if (!db_upper_rect.empty())
		{
			db_upper_rect.clear();
		}

		if (!db_lower_rect.empty())
		{
			db_lower_rect.clear();
		}

		if (!db_pro_values.empty())
		{
			db_pro_values.clear();
		}

		if (!erased_figure.empty())
		{
			erased_figure.clear();
		}
		if (!added_figure.empty())
		{
			added_figure.clear();
		}
		ShowImage(imagePath);
		DrawOnBuffer();
		//DB_AddImageInfor();
		save_done = false;
		from_db = false;
	}
}

void CLandmarkEditDlg::OnEditPro()
{
	INT_PTR nRes;
	CEditPropertyDlg properties;
	
	if (db_pro_values.find(cur_figure)==db_pro_values.end())
	{
		map<string, string, greater<string>> tmp;
		for (int i = 0; i < FieldName.size();++i)
		{
			tmp[FieldName[i]] = "";
		}
		db_pro_values[cur_figure] = tmp;
	}
	PropertyValue = db_pro_values[cur_figure];
	nRes = properties.DoModal();
	if (nRes == IDOK)
	{
		db_pro_values[cur_figure] = PropertyValue;
		cur_figure = -1;
	}	
}

void CLandmarkEditDlg::OnDeleteFigure()
{
	//DB_DeleteOneFigure(cur_figure);
	erased_figure.push_back(cur_figure);
	db_rect.erase(cur_figure);
	if (db_upper_rect.find(cur_figure)!=db_upper_rect.end())
	{
		db_upper_rect.erase(cur_figure);
	}
	if (db_lower_rect.find(cur_figure) != db_lower_rect.end())
	{
		db_lower_rect.erase(cur_figure);
	}
	if (db_pro_values.find(cur_figure) != db_pro_values.end())
	{
		db_pro_values.erase(cur_figure);
	}

	move_point = (-1, -1);
	resize_l = (-1, -1);
	resize_u = (-1, -1);
	resize_lt = (-1, -1);
	resize_rt = (-1, -1);
	resize_lb = (-1, -1);
	resize_rb = (-1, -1);
	cur_figure = -1;
	resize_rect = false;
	move_rect = false;

	Invalidate();
}

void CLandmarkEditDlg::OnBnClickedNextPic()
{
	if (!from_db)
	{
		if (!save_done)
		{
			AfxMessageBox(L"当前图片未保存!!!", MB_OK | MB_ICONERROR);
		}
		else
		{
			Images = GetRemainingImages(basePath);
			if (!Images.empty())
			{
				currentImageName = Images[0];
				SetDlgItemText(IDC_IMAGE_NAME, currentImageName);
				imagePath = basePath + "\\" + currentImageName;

				int len = WideCharToMultiByte(CP_ACP, 0, imagePath, -1, NULL, 0, NULL, NULL);
				PicPath = new char[len + 1];
				WideCharToMultiByte(CP_ACP, 0, imagePath, -1, PicPath, len, NULL, NULL);

				if (!db_rect.empty())
				{
					db_rect.clear();
				}
				if (!db_upper_rect.empty())
				{
					db_upper_rect.clear();
				}
				if (!db_lower_rect.empty())
				{
					db_lower_rect.clear();
				}
				if (!db_pro_values.empty())
				{
					db_pro_values.clear();
				}
				if (!erased_figure.empty())
				{
					erased_figure.clear();
				}
				if (!added_figure.empty())
				{
					added_figure.clear();
				}
				save_done = false;
				ShowImage(imagePath);
				DrawOnBuffer();
				//DB_AddImageInfor();
			}
			else
			{
				currentImageName.Empty();
				imagePath.Empty();

				if (!db_rect.empty())
				{
					db_rect.clear();
				}
				if (!db_upper_rect.empty())
				{
					db_upper_rect.clear();
				}
				if (!db_lower_rect.empty())
				{
					db_lower_rect.clear();
				}
				if (!db_pro_values.empty())
				{
					db_pro_values.clear();
				}
				if (!erased_figure.empty())
				{
					erased_figure.clear();
				}
				if (!added_figure.empty())
				{
					added_figure.clear();
				}
				AfxMessageBox(L"当前文件夹没有未处理图片", MB_OK | MB_ICONINFORMATION);
			}
		}	
	}
}

void CLandmarkEditDlg::OnBnClickedSearch()
{
	//if (!DB_IsAdmin(op_id))
	//{
	//	AfxMessageBox(L"您没有权限查询！", MB_OK | MB_ICONERROR);
	//}
	//else
	//{
	m_listCtrl.DeleteAllItems();
	m_listCtrl.DeleteColumn(0);

	CTime ct1, ct2;
	m_dateStart.GetTime(ct1);
	CString event_date_start = ct1.Format("%Y-%m-%d");
	m_dateEnd.GetTime(ct2);
	CString event_date_end = ct2.Format("%Y-%m-%d");

	CEdit *edit_op = (CEdit*)GetDlgItem(IDC_OPERATOR_ID);
	CString operator_id;
	edit_op->GetWindowTextW(operator_id);

	std::vector<CString> pathes = DB_GetImagePath(event_date_start, event_date_end, operator_id);
	if (!pathes.empty())
	{
		m_listCtrl.InsertColumn(0, L"图片路径", LVCFMT_LEFT, 150);
		for (int i = 0; i < pathes.size(); ++i)
		{
			m_listCtrl.InsertItem(i, NULL);
			m_listCtrl.SetItemText(i, 0, pathes[i]);
		}
	}
	//}
	
}

void CLandmarkEditDlg::OnNMClickList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	int nIndex = pNMListView->iItem;
	
	if (nIndex >= 0)
	{
		from_db = true;
		save_done = false;
		CRect rect(LEFT_BEGIN, TOP_BEGIN, LEFT_BEGIN + IM_WIDTH, TOP_BEGIN + IM_HEIGHT);
		pDC->FillSolidRect(&rect, pDC->GetBkColor());

		CString file_path = m_listCtrl.GetItemText(nIndex, 0);
		SetDlgItemText(IDC_IMAGE_NAME, file_path);

		if (!db_rect.empty())
		{
			db_rect.clear();
		}
		if (!db_upper_rect.empty())
		{
			db_upper_rect.clear();
		}
		if (!db_lower_rect.empty())
		{
			db_lower_rect.clear();
		}
		if (!db_pro_values.empty())
		{
			db_pro_values.clear();
		}
		if (!erased_figure.empty())
		{
			erased_figure.clear();
		}
		if (!added_figure.empty())
		{
			added_figure.clear();
		}
		imagePath = save_path + "\\" + file_path;

		int len = WideCharToMultiByte(CP_ACP, 0, imagePath, -1, NULL, 0, NULL, NULL);
		PicPath = new char[len + 1];
		WideCharToMultiByte(CP_ACP, 0, imagePath, -1, PicPath, len, NULL, NULL);

		CFileFind fileFinder;
		if (fileFinder.FindFile(imagePath))
		{
			ShowImage(imagePath);
			DB_ReadData(imagePath);
			DrawOnBuffer();
		}
	}
}

void CLandmarkEditDlg::OnBnClickedSave()
{
	SaveOption();
}

void CLandmarkEditDlg::OnAddOperator()
{
	if (!DB_IsAdmin(op_id))
	{
		AfxMessageBox(L"您没有权限添加用户", MB_OK | MB_ICONERROR);
	}
	else
	{
		CAddOperatorDlg add;
		INT_PTR res = add.DoModal();
		if (IDOK == res)
		{
			UpdateData(true);
			if (!add.operator_id.IsEmpty() && !add.password.IsEmpty())
			{
				DB_InsertOperator(add.operator_id,add.op_name, add.password, L"normal", L"logout");
			}
		}
	}
}

void CLandmarkEditDlg::OnExit()
{
	if (!save_done)
	{
		int res = AfxMessageBox(L"当前操作未保存，是否保存？", MB_YESNOCANCEL | MB_ICONERROR);
		if (res == IDYES)
		{
			SaveOption();
			DB_ChangeState(op_id, L"logout");
			EndDialog(0);
		}
		else if (res == IDNO)
		{
			DB_ChangeState(op_id, L"logout");
			EndDialog(0);
		}
	}
	else
	{
		DB_ChangeState(op_id, L"logout");
		CDialogEx::OnClose();
	}
}

void CLandmarkEditDlg::OnClose()
{
	if (!save_done)
	{
		int res = AfxMessageBox(L"当前操作未保存，是否保存？", MB_YESNOCANCEL | MB_ICONERROR);
		if (res == IDYES)
		{
			SaveOption();
			DB_ChangeState(op_id, L"logout");
			EndDialog(0);
		}
		else if (res == IDNO)
		{
			DB_ChangeState(op_id, L"logout");
			EndDialog(0);
		}
	}
	else
	{
		DB_ChangeState(op_id, L"logout");
		CDialogEx::OnClose();
	}
}

void CLandmarkEditDlg::OnChangePwd()
{
	CChangePwdDlg change_pwd;
	INT_PTR nRes = change_pwd.DoModal();
	if (IDOK == nRes)
	{
		UpdateData(true);
		if (change_pwd.old_pwd == DB_GetPwd(op_id))
		{
			DB_SetNewPwd(op_id, change_pwd.new_pwd);
		}
		else
		{
			AfxMessageBox(L"原始密码填写错误，无法更改密码！！！", MB_OK | MB_ICONERROR);
		}
	}
}

void CLandmarkEditDlg::OnAddPro()
{
	if (!DB_IsAdmin(op_id))
	{
		AfxMessageBox(L"您没有权限添加属性", MB_OK | MB_ICONERROR);
	}
	else
	{
		CAddPropertyDlg add_pro;
		INT_PTR nRes = add_pro.DoModal();
		if (IDOK == nRes)
		{
			UpdateData(true);
			DB_AddProperty(add_pro.selected, add_pro.property_name);
			string new_pro_value = CStringA(add_pro.property_value);
			string new_pro_name = CStringA(add_pro.property_name);
			FieldName.push_back(new_pro_name);

			std::vector<string> new_pro_values = split(new_pro_value, ",");
			string cfgPath = save_path + "\\sqlite.cfg";
			ofstream outfile(cfgPath, ios::app);
			if (add_pro.selected == 0)
			{
				pro_values[new_pro_name] = new_pro_values;
				outfile << endl << new_pro_name << ":" << new_pro_value;
			}
			else if (add_pro.selected == 1)
			{
				string upper_pro = "upper_" + CStringA(add_pro.property_name);
				string lower_pro = "lower_" + CStringA(add_pro.property_name);
				pro_values[upper_pro] = new_pro_values;
				pro_values[lower_pro] = new_pro_values;
				outfile << endl << upper_pro << ":" << new_pro_value;
				outfile << endl << lower_pro << ":" << new_pro_value;
			}
			outfile.close();
		}
	}
}

void CLandmarkEditDlg::OnBnClickedDelPic()
{
	//todo 管理员
	if (DB_IsAdmin(op_id) && from_db)
	{
		if (AfxMessageBox(L"该图片相关信息以及图片文件都将被删除！！", MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
		{
			if (!db_rect.empty())
			{
				db_rect.clear();
			}

			if (!db_upper_rect.empty())
			{
				db_upper_rect.clear();
			}

			if (!db_lower_rect.empty())
			{
				db_lower_rect.clear();
			}

			if (!db_pro_values.empty())
			{
				db_pro_values.clear();
			}
			if (!erased_figure.empty())
			{
				erased_figure.clear();
			}
			if (!added_figure.empty())
			{
				added_figure.clear();
			}
			DB_DelCurrentImage(existed_image_id);
			save_done = true;
			DeleteFile(imagePath);
			Invalidate();
		}
	}
	else if (!from_db)
	{
		if (!imagePath.IsEmpty())
		{
			if (AfxMessageBox(L"该图片相关信息将被删除！", MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
			{
				if (!db_rect.empty())
				{
					db_rect.clear();
				}
				if (!db_upper_rect.empty())
				{
					db_upper_rect.clear();
				}
				if (!db_lower_rect.empty())
				{
					db_lower_rect.clear();
				}
				if (!db_pro_values.empty())
				{
					db_pro_values.clear();
				}
				if (!erased_figure.empty())
				{
					erased_figure.clear();
				}
				if (!added_figure.empty())
				{
					added_figure.clear();
				}
				save_done = true;
				DeleteFile(imagePath);
				imagePath.Empty();
				Invalidate();
			}
		}
	}
	
}

void CLandmarkEditDlg::OnDelOperator()
{
	if (!DB_IsAdmin(op_id))
	{
		AfxMessageBox(L"您没有权限删除用户", MB_OK | MB_ICONERROR);
	}
	else
	{
		CDelOperatorDlg del_op;
		operators = DB_GetOperators();
		INT_PTR nRes = del_op.DoModal();
		if (IDOK == nRes)
		{
			DB_DelOperotors(to_del_ops);
		}
	}
}

void CLandmarkEditDlg::OnBodyRect()
{
	int res = GetCurrentFigure(db_rect, rect_start, rect_end);

	if (res!=0)
	{
		AfxMessageBox(L"已经标注该人体框！", MB_OK | MB_ICONERROR);
	  /*rect_start = (-1, -1);
		rect_end = (-1, -1);
		move_point = (-1, -1);
		Invalidate();*/
	}
	else
	{
		//figure_amount = DB_GetFigureAmount();

		int l = (rect_start.x - LEFT_BEGIN) * image_width / IM_WIDTH;
		int t = (rect_start.y - TOP_BEGIN) * image_height / IM_HEIGHT;
		int r = (rect_end.x - LEFT_BEGIN) * image_width / IM_WIDTH;
		int b = (rect_end.y - TOP_BEGIN) *image_height / IM_HEIGHT;

		//DB_InsertOneFigure(l, t, r - l, b - t, existed_image_id);
		int figure = 1;
		map<int, CRect>::reverse_iterator it = db_rect.rbegin();
		if (it!=db_rect.rend())
		{
			figure = it->first + 1;
		}
		added_figure.push_back(figure);

		CRect rect(l, t, r, b);
		db_rect[figure] = rect;
		rect_start = CPoint(-1, -1);
		rect_end = CPoint(-1, -1);

		move_point = (-1, -1);
		resize_rect = false;
		move_rect = false;
		resize_l = (-1, -1);
		resize_u = (-1, -1);
		resize_lt = (-1, -1);
		resize_rt = (-1, -1);
		resize_lb = (-1, -1);
		resize_rb = (-1, -1);
		cur_figure = -1;

		Invalidate();
	}
	
}

void CLandmarkEditDlg::OnUpperRect()
{
	int res = GetCurrentFigure(db_rect, rect_start, rect_end);
	//std::vector<int> BodyRect = DB_GetRects(res);
	if (res == 0)
	{
		AfxMessageBox(L"请先标注人体全身矩形框", MB_OK | MB_ICONERROR);
	}
	else
	{
		int t = (rect_start.y - TOP_BEGIN) * image_height / IM_HEIGHT;
		int b = (rect_end.y - TOP_BEGIN) *image_height / IM_HEIGHT;

		//DB_InsertUpperRect(BodyRect[0], t, BodyRect[2], b - t, res);
		//DB_InsertLowerRect(BodyRect[0], b, BodyRect[2], BodyRect[1] + BodyRect[3] - b, res);
		db_upper_rect[res] = CRect(db_rect[res].left, t, db_rect[res].right, b);
		db_lower_rect[res] = CRect(db_rect[res].left, b, db_rect[res].right, db_rect[res].bottom);
		rect_start = CPoint(-1, -1);
		rect_end = CPoint(-1, -1);
		move_point = CPoint(-1, -1);
		move_rect = false;

		Invalidate();
	}
}

void CLandmarkEditDlg::OnMovRect()
{
	////todo
	//std::vector<int> BodyRect;
	//
	//= DB_GetRects(cur_figure);
	//int x = BodyRect[0] + BodyRect[2] / 2;
	//int r = BodyRect[0] + BodyRect[2];
	//int b = BodyRect[1] + BodyRect[3];

	//move_point = CPoint(LEFT_BEGIN + x * IM_WIDTH / image_width, TOP_BEGIN + BodyRect[1] * IM_HEIGHT / image_height);
	//resize_lt = CPoint(LEFT_BEGIN + BodyRect[0] * IM_WIDTH / image_width, TOP_BEGIN + BodyRect[1] * IM_HEIGHT / image_height);
	//resize_rt = CPoint(LEFT_BEGIN + r * IM_WIDTH / image_width, TOP_BEGIN + BodyRect[1] * IM_HEIGHT / image_height);
	//resize_lb = CPoint(LEFT_BEGIN + BodyRect[0] * IM_WIDTH / image_width, TOP_BEGIN +b * IM_HEIGHT / image_height);
	//resize_rb = CPoint(LEFT_BEGIN + r * IM_WIDTH / image_width, TOP_BEGIN + b * IM_HEIGHT / image_height);

	//if (BodyRect[5] != 0)
	//{
	//	resize_u = CPoint(LEFT_BEGIN + x * IM_WIDTH / image_width, TOP_BEGIN + BodyRect[5] * IM_HEIGHT / image_height);
	//	resize_l = CPoint(LEFT_BEGIN + x * IM_WIDTH / image_width, TOP_BEGIN + BodyRect[9] * IM_HEIGHT / image_height);
	//}
	int l = (db_rect[cur_figure].left + db_rect[cur_figure].right) / 2;
	move_point = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
	resize_lt = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
	resize_rt = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].top * IM_HEIGHT / image_height);
	resize_lb = CPoint(LEFT_BEGIN + db_rect[cur_figure].left * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
	resize_rb = CPoint(LEFT_BEGIN + db_rect[cur_figure].right * IM_WIDTH / image_width, TOP_BEGIN + db_rect[cur_figure].bottom * IM_HEIGHT / image_height);
	if (db_upper_rect.find(cur_figure)!=db_upper_rect.end())
	{
		resize_u = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].top * IM_HEIGHT / image_height);
		resize_l = CPoint(LEFT_BEGIN + l * IM_WIDTH / image_width, TOP_BEGIN + db_upper_rect[cur_figure].bottom * IM_HEIGHT / image_height);
	}
	Invalidate();
}

void CLandmarkEditDlg::OnCancelRect()
{
	rect_start = CPoint(-1, -1);
	rect_end = CPoint(-1, -1);
	move_point = CPoint(-1, -1);
	move_rect = false;
	Invalidate();
}

void CLandmarkEditDlg::ShowImage(CString imagePath)
{
	CFileFind fileFinder;
	if (fileFinder.FindFile(imagePath))
	{
		CImage  image;
		image.Load(imagePath);

		image_width = image.GetWidth();
		image_height = image.GetHeight();
	
		pDC->SetStretchBltMode(HALFTONE);
		CRect rect(LEFT_BEGIN, TOP_BEGIN, IM_WIDTH + LEFT_BEGIN, IM_HEIGHT + TOP_BEGIN);
		image.Draw(pDC->m_hDC, rect);

		image.Destroy();
	}
}

void CLandmarkEditDlg::DrawOnBuffer()
{
	CRgn rgn;
	rgn.CreateRectRgn(LEFT_BEGIN, TOP_BEGIN, LEFT_BEGIN + IM_WIDTH, TOP_BEGIN + IM_HEIGHT);
	pDC->SelectClipRgn(&rgn);

	dc_mem.CreateCompatibleDC(pDC);
	bitmap.CreateCompatibleBitmap(pDC, MainRect.Width(), MainRect.Height());
	CBitmap *pOldBit = dc_mem.SelectObject(&bitmap);
	dc_mem.FillSolidRect(MainRect, pDC->GetBkColor());

	CFileFind fileFinder;
	CImage image;
	if (fileFinder.FindFile(imagePath))
	{
		image.Load(imagePath);
		dc_mem.SetStretchBltMode(HALFTONE);
		CRect rect(LEFT_BEGIN, TOP_BEGIN, IM_WIDTH + LEFT_BEGIN, IM_HEIGHT + TOP_BEGIN);
		image.Draw(dc_mem.m_hDC, rect);
	}
	if (rect_start != CPoint(-1, -1) && rect_end != CPoint(-1, -1))
	{
		DrawRectangle(rect_start, rect_end);
	}

	if (!db_upper_rect.empty())
	{
		std::map<int, CRect>::iterator it;
		for (it = db_upper_rect.begin(); it != db_upper_rect.end(); ++it)
		{
			CRect rect = it->second;
			CPoint lt(rect.left * IM_WIDTH / image_width + LEFT_BEGIN, rect.top * IM_HEIGHT / image_height + TOP_BEGIN);
			CPoint rt(rect.right * IM_WIDTH / image_width + LEFT_BEGIN, rect.top * IM_HEIGHT / image_height + TOP_BEGIN);
			CPoint lb(rect.left * IM_WIDTH / image_width + LEFT_BEGIN, rect.bottom * IM_HEIGHT / image_height + TOP_BEGIN);
			CPoint rb(rect.right * IM_WIDTH / image_width + LEFT_BEGIN, rect.bottom * IM_HEIGHT / image_height + TOP_BEGIN);
			DrawLine(PS_SOLID, lt, rt, RGB(255, 0, 0));
			DrawLine(PS_SOLID, lb, rb, RGB(255, 0, 0));
		}
	}
	if (!db_rect.empty())
	{
		std::map<int, CRect>::iterator it;
		for (it = db_rect.begin(); it != db_rect.end(); ++it)
		{
			CRect rect = it->second;
			CRect dr_rect(rect.left * IM_WIDTH / image_width + LEFT_BEGIN, rect.top * IM_HEIGHT / image_height + TOP_BEGIN,
				rect.right * IM_WIDTH / image_width + LEFT_BEGIN, rect.bottom * IM_HEIGHT / image_height + TOP_BEGIN);
			DrawRectangle(dr_rect, RGB(0, 255, 0));
		}
	}

	if (move_point != CPoint(-1, -1))
	{
		DrawResizePoints();
	}

	pDC->BitBlt(0, 0, MainRect.Width(), MainRect.Height(), &dc_mem, 0, 0, SRCCOPY);

	pDC->SelectClipRgn(NULL);
	dc_mem.DeleteDC();
	bitmap.DeleteObject();
	image.Destroy();
}

void CLandmarkEditDlg::DrawRectangle(CRect rect, COLORREF color)
{
	CPen pen(PS_SOLID, 2, color);
	CPen *pOldPen = dc_mem.SelectObject(&pen);

	CBrush *pBrush = CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH));
	CBrush *pOldBrush = dc_mem.SelectObject(pBrush);
	dc_mem.Rectangle(rect);

	dc_mem.SelectObject(pOldPen);
	dc_mem.SelectObject(pOldBrush);
}

void CLandmarkEditDlg::DrawRectangle(CPoint X, CPoint Y)
{
	CPen pen(PS_SOLID, 2, RGB(0, 255, 0));
	CPen *pOldPen = dc_mem.SelectObject(&pen);

	CBrush *pBrush = CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH));
	CBrush *pOldBrush = dc_mem.SelectObject(pBrush);
	dc_mem.Rectangle(CRect(X, Y));

	dc_mem.SelectObject(pOldPen);
	dc_mem.SelectObject(pOldBrush);
}

void CLandmarkEditDlg::DrawEclipse(int x, int y, int r, COLORREF color)
{
	CBrush brush, *oldbrush;
	brush.CreateSolidBrush(color);
	oldbrush = dc_mem.SelectObject(&brush);
	dc_mem.Ellipse(x - r, y - r, x + r, y + r);
	dc_mem.SelectObject(oldbrush);
}

void CLandmarkEditDlg::DrawLine(int nPenStyle, CPoint x, CPoint y, COLORREF color)
{
	CPen pen(nPenStyle, 2, color);
	dc_mem.SelectObject(&pen);
	dc_mem.MoveTo(x);
	dc_mem.LineTo(y);
}

void CLandmarkEditDlg::DrawResizePoints()
{
	DrawEclipse(move_point.x, move_point.y, 4, RGB(255, 255, 255));
	DrawLine(PS_SOLID, CPoint(move_point.x, move_point.y - 10), CPoint(move_point.x, move_point.y + 10), RGB(255, 255, 255));
	DrawLine(PS_SOLID, CPoint(move_point.x - 10, move_point.y), CPoint(move_point.x + 10, move_point.y), RGB(255, 255, 255));

	DrawEclipse(resize_lt.x, resize_lt.y, 4, RGB(255, 255, 255));
	DrawLine(PS_SOLID, CPoint(resize_lt.x, resize_lt.y - 10), CPoint(resize_lt.x, resize_lt.y + 10), RGB(255, 255, 255));
	DrawLine(PS_SOLID, CPoint(resize_lt.x - 10, resize_lt.y), CPoint(resize_lt.x + 10, resize_lt.y), RGB(255, 255, 255));
	
	DrawEclipse(resize_rt.x, resize_rt.y, 4, RGB(255, 255, 255));
	DrawLine(PS_SOLID, CPoint(resize_rt.x, resize_rt.y - 10), CPoint(resize_rt.x, resize_rt.y + 10), RGB(255, 255, 255));
	DrawLine(PS_SOLID, CPoint(resize_rt.x - 10, resize_rt.y), CPoint(resize_rt.x + 10, resize_rt.y), RGB(255, 255, 255));

	DrawEclipse(resize_lb.x, resize_lb.y, 4, RGB(255, 255, 255));
	DrawLine(PS_SOLID, CPoint(resize_lb.x, resize_lb.y - 10), CPoint(resize_lb.x, resize_lb.y + 10), RGB(255, 255, 255));
	DrawLine(PS_SOLID, CPoint(resize_lb.x - 10, resize_lb.y), CPoint(resize_lb.x + 10, resize_lb.y), RGB(255, 255, 255));

	DrawEclipse(resize_rb.x, resize_rb.y, 4, RGB(255, 255, 255));
	DrawLine(PS_SOLID, CPoint(resize_rb.x, resize_rb.y - 10), CPoint(resize_rb.x, resize_rb.y + 10), RGB(255, 255, 255));
	DrawLine(PS_SOLID, CPoint(resize_rb.x - 10, resize_rb.y), CPoint(resize_rb.x + 10, resize_rb.y), RGB(255, 255, 255));

	DrawEclipse(resize_u.x, resize_u.y, 4, RGB(255, 255, 255));
	DrawLine(PS_SOLID, CPoint(resize_u.x, resize_u.y - 10), CPoint(resize_u.x, resize_u.y + 10), RGB(255, 255, 255));
	DrawLine(PS_SOLID, CPoint(resize_u.x - 10, resize_u.y), CPoint(resize_u.x + 10, resize_u.y), RGB(255, 255, 255));

	DrawEclipse(resize_l.x, resize_l.y, 4, RGB(255, 255, 255));
	DrawLine(PS_SOLID, CPoint(resize_l.x, resize_l.y - 10), CPoint(resize_l.x, resize_l.y + 10), RGB(255, 255, 255));
	DrawLine(PS_SOLID, CPoint(resize_l.x - 10, resize_l.y), CPoint(resize_l.x + 10, resize_l.y), RGB(255, 255, 255));
}

std::map<string, COLORREF> CLandmarkEditDlg::InitColorRGB()
{
	std::map<string, COLORREF> clr;
	clr["red"] = RGB(255,0,0);
	clr["green"] = RGB(0, 255, 0);
	clr["blue"] = RGB(0, 0, 255);
	clr["cyan"] = RGB(0, 255, 255);
	clr["magenta"] = RGB(255, 0, 255);
	clr["yellow"] = RGB(255, 255, 0);
	clr["white"] = RGB(255, 255, 255);
	clr["black"] = RGB(0, 0, 0);
	clr["gray"] = RGB(190,190,190);
	return clr;
}

std::map<string, cv::Scalar> CLandmarkEditDlg::InitColorScalar()
{
	std::map<string, cv::Scalar> res;
	res["red"] = cv::Scalar(0,0,255);
	res["green"] = cv::Scalar(0, 255, 0);
	res["blue"] = cv::Scalar(255, 0, 0);
	res["cyan"] = cv::Scalar(255, 255, 0);
	res["magenta"] = cv::Scalar(255, 0, 255);
	res["yellow"] = cv::Scalar(0, 255, 255);
	res["white"] = cv::Scalar(255, 255, 255);
	res["black"] = cv::Scalar(0, 0, 0);
	res["gray"] = cv::Scalar(190, 190, 190);
	return res;
}

std::vector<CString> CLandmarkEditDlg::GetRemainingImages(CString basePath)
{
	std::vector<CString> Images;
	CFileFind finder;
	BOOL working = finder.FindFile(basePath + "\\*.jpg");
	while (working)
	{
		working = finder.FindNextFile();
		if (finder.IsDots())
			continue;
		if (!finder.IsDirectory())
		{
			Images.push_back(finder.GetFileName());
		}
	}
	finder.Close();

	return Images;
}

void CLandmarkEditDlg::InitProValues()
{
	string cfgPath = save_path + "\\sqlite.cfg";
	ifstream infile(cfgPath);
	string lines;
	while (getline(infile, lines))
	{
		string field_name = split(lines,":")[0];
		FieldName.push_back(field_name);
		string field_values = split(lines, ":")[1];
		std::vector<string> values = split(field_values, ",");
		pro_values[field_name] = values;
	}
	infile.close();
	/*std::map<string, std::vector<string>> res;
	res[FieldName[0]] = std::vector<string>({ "red", "green", "blue", "cyan" ,"magenta", "yellow", "white", "black" });
	res[FieldName[1]] = std::vector<string>({ "red", "green", "blue", "cyan", "magenta", "yellow", "white", "black" });
	res[FieldName[2]] = std::vector<string>({ "yes", "no" });
	res[FieldName[3]] = std::vector<string>({ "yes", "no" });*/
}

void CLandmarkEditDlg::SaveOption()
{
	if (!save_done)
	{
		if (db_pro_values.size() < db_rect.size() || db_upper_rect.size() < db_rect.size())
		{
			AfxMessageBox(L"未标注衣物颜色或上下半身！", MB_OK | MB_ICONERROR);
		}
		else
		{
			int flag = 1;
			map<int, map<string, string, greater<string>>>::iterator it;
			for (it = db_pro_values.begin(); it != db_pro_values.end(); ++it)
			{
				map<string, string, greater<string>> pro = it->second;
				map<string, string, greater<string>>::iterator iter;
				for (iter = pro.begin(); iter != pro.end(); ++iter)
				{
					if (iter->second.size() == 0)
					{
						AfxMessageBox(L"未标注衣物颜色，无法保存！", MB_OK | MB_ICONERROR);
						flag = 0;
						break;
					}
				}
			}
			if (flag == 1)
			{
				if (!imagePath.IsEmpty())
				{
					if (!from_db)
					{
						DB_WriteData();
						CRect rect(LEFT_BEGIN, TOP_BEGIN, LEFT_BEGIN + IM_WIDTH, TOP_BEGIN + IM_HEIGHT);
						pDC->FillSolidRect(&rect, pDC->GetBkColor());

						Merge(currentImageName);
						save_done = true;

						if (!db_rect.empty())
						{
							db_rect.clear();
						}

						if (!db_upper_rect.empty())
						{
							db_upper_rect.clear();
						}

						if (!db_lower_rect.empty())
						{
							db_lower_rect.clear();
						}

						if (!db_pro_values.empty())
						{
							db_pro_values.clear();
						}

						if (!erased_figure.empty())
						{
							erased_figure.clear();
						}
						if (!added_figure.empty())
						{
							added_figure.clear();
						}
					}
					else
					{
						DB_UpdateData();
						save_done = true;
					}
				}
			}
		}
	}
}

void CLandmarkEditDlg::InitRects(int image_id)
{
	std::vector<int> figures = DB_GetFigures(image_id);
	figure_amount = figures.size();
	for (int i = 0; i < figures.size(); ++i)
	{
		std::vector<int> rects = DB_GetRects(figures[i]);
		map<string, string, greater<string>> pro = DB_GetPropertyValue(figures[i]);
		db_pro_values[figures[i]] = pro;
		db_rect[figures[i]] = CRect(rects[0], rects[1], rects[2] + rects[0], rects[3] + rects[1]);
		db_upper_rect[figures[i]] = CRect(rects[4], rects[5], rects[6] + rects[4], rects[7] + rects[5]);
		db_lower_rect[figures[i]] = CRect(rects[8], rects[9], rects[10] + rects[8], rects[11] + rects[9]);
	}
}

int CLandmarkEditDlg::GetCurFigureId(CPoint x)
{
	int res = 0;
	int X = (x.x - LEFT_BEGIN) * image_width / IM_WIDTH;
	int Y = (x.y - TOP_BEGIN) * image_height / IM_HEIGHT;

	std::map<int, CRect>::iterator it;
	for (it = db_rect.begin(); it != db_rect.end(); ++it)
	{
		CRect rect = it->second;
		if (rect.left <= X && rect.top <= Y && rect.right >= X && rect.bottom >= Y)
		{
			res = it->first;
			break;
		}
	}
	return res;
}

int CLandmarkEditDlg::DB_GetFigureAmount()
{
	int amount;
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		char *sql = "select seq from sqlite_sequence where name='Figures'";
		char** pResult;
		int nRow;
		int nCol;
		rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, NULL);
		if (rc == SQLITE_OK)
		{
			if (nRow < 1){
				amount = 0;
			}
			else{
				amount = atoi(pResult[nCol]);
			}
		}
		sqlite3_free_table(pResult);
	}
	sqlite3_close(db);
	return amount;
}

int CLandmarkEditDlg::DB_GetImageAmount()
{
	int amount;
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		char *sql = "select seq from sqlite_sequence where name='Images'";
		char** pResult;
		int nRow;
		int nCol;
		rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, NULL);
		if (rc == SQLITE_OK)
		{
			if (nRow < 1){
				amount = 0;
			}
			else{
				amount = atoi(pResult[nCol]);
			}
		}
		sqlite3_free_table(pResult);
	}
	sqlite3_close(db);

	return amount; 
}

int CLandmarkEditDlg::DB_ImageExisted(CString imagePath)
{
	int image_id;
	char sql[256];
	string path_db = PathToDB(imagePath);
	sprintf(sql, "select image_id from Images where file_path='%s'", path_db.c_str());

	char** pResult;
	int nRow;
	int nCol;
	int rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, NULL);
	if (rc == SQLITE_OK)
	{
		if (nRow < 1){
			image_id = 0;
		}
		else{
			image_id = atoi(pResult[nCol]);
		}
			
	}
	sqlite3_free_table(pResult);

	return image_id;
}

std::vector<CString> CLandmarkEditDlg::DB_GetImagePath(CString event_date_start, CString event_date_end, CString operator_id)
{
	std::vector<CString> pathes;
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		char sql[256];
		string s = CStringA(event_date_start);
		string e = CStringA(event_date_end);
		if (operator_id.IsEmpty())
		{
			sprintf(sql, "select file_path from Images where event_date >= '%s' and event_date <= '%s'", s.c_str(), e.c_str());
		}
		else
		{
			if (event_date_end < event_date_start)
			{
				sprintf(sql, "select file_path from Images where operator_id = '%d'", _ttoi(operator_id));
			}
			else
			{
				sprintf(sql, "select file_path from Images where event_date >= '%s' and event_date <= '%s' and operator_id = '%d'", s.c_str(), e.c_str(), _ttoi(operator_id));
			}
		}

		char** pResult;
		int nRow;
		int nCol;
		rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, NULL);
		if (rc == SQLITE_OK)
		{
			if (nRow >= 1){
				for (int i = nCol; i < nCol * (nRow + 1); ++i)
				{
					USES_CONVERSION;
					CString str = A2W(pResult[i]);
					pathes.push_back(str);
				}
			}
		}
		sqlite3_free_table(pResult);
	}
	sqlite3_close(db);
	return pathes;
}

bool CLandmarkEditDlg::DB_DelCurrentImage(int image_id)
{
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		stringstream ss;
		ss << image_id;
		string sql1 = "delete from Images where image_id = " + ss.str();

		const char *sql_del_pic = sql1.c_str();
		rc = sqlite3_exec(db, sql_del_pic, NULL, NULL, NULL);

		string sql2 = "delete from Figures where image_id = " + ss.str();
		const char *sql_del_faces = sql2.c_str();
		rc = sqlite3_exec(db, sql_del_faces, NULL, NULL, NULL);
	}
	sqlite3_close(db);

	if (rc == SQLITE_OK)
	{
		return true;
	}
	else
	{
		return false;
	}
}

string CLandmarkEditDlg::DB_GetFigureCreateSql()
{
	string res = "";
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		char *sql = "select sql from sqlite_master where tbl_name = 'Figures' and type = 'table'";
		char** pResult;
		int nRow;
		int nCol;
		rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, NULL);
		if (rc == SQLITE_OK)
		{
			if (nRow >= 1)
			{
				res = pResult[nCol];
			}
		}
		sqlite3_free_table(pResult);
	}
	sqlite3_close(db);
	return res;
}

std::vector<string> CLandmarkEditDlg::GetFigureFieldName(string res)
{
	std::vector<string> r = split(res, ",");
	std::vector<string> textField;
	for (int i = 14; i < r.size(); ++i)
	{
		std::vector<string> tp = split(r[i], "\"");

		if (tp.size() == 2)
		{
			textField.push_back(tp[0]);
		}
		else if (tp.size() > 2)
		{
			textField.push_back(tp[1]);
		}
	}
	return textField;
}

int CLandmarkEditDlg::GetCurrentFigure(std::map<int, CRect> rect, CPoint x, CPoint y)
{
	int res = 0;
	int l = (x.x - LEFT_BEGIN) * image_width / IM_WIDTH;
	int t = (x.y - TOP_BEGIN) * image_height / IM_HEIGHT;
	int r = (y.x - LEFT_BEGIN) * image_width / IM_WIDTH;
	int b = (y.y - TOP_BEGIN) *image_height / IM_HEIGHT;

	std::map<int, CRect>::iterator it;
	for (it = rect.begin(); it != rect.end(); ++it)
	{
		CRect rec = it->second;
		if (rec.left <= l && rec.top <= t && rec.right >= r && rec.bottom >= b)
		{
			res = it->first;
			break;
		}
	}
	return res;
}

std::map<string, string, greater<string>> CLandmarkEditDlg::DB_GetPropertyValue(int figure_id)
{
	std::map<string, string, greater<string>> res;
	int rc;
	stringstream ss;
	ss << figure_id;
	string sql_search = "select * from Figures where figure_id=" + ss.str();
	const char *sql = sql_search.c_str();

	char** pResult;
	int nRow;
	int nCol;
	rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, NULL);
	if (rc == SQLITE_OK)
	{
		if (nRow >= 1)
		{
			for (int i = 14; i < nCol; ++i)
			{
				res[pResult[i]] = pResult[i + nCol];
			}
		}
	}
	sqlite3_free_table(pResult);
	return res;
}

std::vector<int> CLandmarkEditDlg::DB_GetFigures(int image_id)
{
	std::vector<int> res;
	int rc ;
	stringstream ss;
	ss << image_id;
	string sql_search = "select figure_id from Figures where image_id=" + ss.str();
	const char *sql = sql_search.c_str();

	char** pResult;
	int nRow;
	int nCol;
	rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, NULL);
	if (rc == SQLITE_OK)
	{
		if (nRow >= 1)
		{
			for (int i = nCol; i < (nRow + 1) * nCol; ++i)
			{
				res.push_back(atoi(pResult[i]));
			}
		}
	}
	sqlite3_free_table(pResult);
	return res;
}

std::vector<int> CLandmarkEditDlg::DB_GetRects(int figure_id)
{
	std::vector<int> res;
	int rc;
	stringstream ss;
	ss << figure_id;
	string sql_search = "select body_left,body_top,body_width,body_height,upper_left,upper_top,upper_width,\\
		upper_height, lower_left, lower_top, lower_width, lower_height from Figures where figure_id = " + ss.str();

	const char *sql = sql_search.c_str();
	char** pResult;
	int nRow;
	int nCol;
	rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, NULL);
	if (rc == SQLITE_OK)
	{
		if (nRow >= 1)
		{
			for (int i = nCol; i < (nRow + 1) * nCol; ++i)
			{
				res.push_back(atoi(pResult[i]));
			}
		}
	}
	sqlite3_free_table(pResult);
	return res;
}

bool CLandmarkEditDlg::DB_AllowLogin(CString operator_id, CString password)
{
	bool res = false;
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		string op = CStringA(operator_id);
		string sql_password;
		sql_password = "select password, state from Operators where operator_id = " + op;
		const char *sql = sql_password.c_str();

		char** pResult;
		int nRow;
		int nCol;
		rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, NULL);
		if (rc == SQLITE_OK)
		{
			if (nRow >= 1)
			{
				USES_CONVERSION;
				CString pa = A2W(pResult[nCol]);
				string stat = pResult[nCol + 1];
				if (password == Decrypt(pa, 1314))//&& stat == "logout"
				{
					res = true;
				}
			}
		}
		sqlite3_free_table(pResult);
	}
	sqlite3_close(db);

	return res;
}

bool CLandmarkEditDlg::DB_OperatorExisted()
{
	bool res = true;

	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		char *sql = "select * from Operators";

		char** pResult;
		int nRow;
		int nCol;
		rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, NULL);
		if (rc == SQLITE_OK)
		{
			if (nRow < 1)
			{
				res = false;
			}
		}
		sqlite3_free_table(pResult);
	}
	sqlite3_close(db);

	return res;
}

bool CLandmarkEditDlg::DB_InsertOperator(CString operator_id, CString op_name, CString password, CString permission, CString state)
{
	string opid = CStringA(operator_id);
	string pa = CStringA(Encrypt(password, 1314));
	string pe = CStringA(permission);
	string opn = CStringA(op_name);
	string stat = CStringA(state);

	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		string sql_insert;
		sql_insert = "insert into Operators values ('" + opid + "', '" + pa + "', '" + pe + "','" + opn + "','" + stat + "')";

		const char *sql_insert_op = sql_insert.c_str();
		rc = sqlite3_exec(db, sql_insert_op, NULL, NULL, NULL);
	}
	sqlite3_close(db);
	if (rc == SQLITE_OK)
	{
		return true;
	}
	return false;
}

bool CLandmarkEditDlg::DB_IsAdmin(CString operator_id)
{
	bool res = false;
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		string sql_search = "select permission from Operators where operator_id = " + (CStringA)operator_id;

		const char *sql = sql_search.c_str();

		char** pResult;
		int nRow;
		int nCol;
		rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, NULL);
		if (rc == SQLITE_OK)
		{
			if (nRow >= 1)
			{
				string tmp = pResult[nCol];
				if (tmp == "admin")
				{
					res = true;
				}
			}
		}
		sqlite3_free_table(pResult);
	}
	sqlite3_close(db);

	return res;
}

CString CLandmarkEditDlg::DB_GetPwd(CString operator_id)
{
	CString old_pwd;
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		string sql_pwd;
		sql_pwd = "select password from Operators where operator_id = " + (CStringA)operator_id;
		const char *sql = sql_pwd.c_str();

		char** pResult;
		int nRow;
		int nCol;
		rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, NULL);
		if (rc == SQLITE_OK)
		{
			if (nRow >= 1)
			{
				USES_CONVERSION;
				old_pwd = A2W(pResult[nCol]);
			}
		}
		sqlite3_free_table(pResult);
	}
	sqlite3_close(db);

	return Decrypt(old_pwd, 1314);
}

bool CLandmarkEditDlg::DB_SetNewPwd(CString operator_id, CString new_pwd)
{
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		string sql_pwd;
		sql_pwd = "update Operators set password = '" + (CStringA)Encrypt(new_pwd, 1314) + "' where operator_id = " + (CStringA)operator_id;
		const char *sql = sql_pwd.c_str();
		rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
	}
	sqlite3_close(db);
	if (rc == SQLITE_OK)
	{
		return true;
	}
	return false;
}

bool CLandmarkEditDlg::DB_AddProperty(int type, CString pro_name)
{
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		if (type == 0)
		{
			string sql = "alter table Figures add column \"" + (CStringA)pro_name + "\" text default ''";
			const char *sql_add_pro = sql.c_str();
			rc = sqlite3_exec(db, sql_add_pro, NULL, NULL, NULL);
		}
		else if (type == 1)
		{
			string u = "upper_" + (CStringA)pro_name;
			string l = "lower_" + (CStringA)pro_name;
			string sql1 = "alter table Figures add column \"" + u + "\" text default ''";
			string sql2 = "alter table Figures add column \"" + l + "\" text default ''";
			const char *sql_add_pro1 = sql1.c_str();
			rc = sqlite3_exec(db, sql_add_pro1, NULL, NULL, NULL);

			const char *sql_add_pro2 = sql2.c_str();
			rc = sqlite3_exec(db, sql_add_pro2, NULL, NULL, NULL);
		}
	}
	sqlite3_close(db);

	if (rc == SQLITE_OK)
	{
		return true;
	}
	else
	{
		return false;
	}
}

std::map<string, string> CLandmarkEditDlg::DB_GetOperators()
{
	std::map<string, string> operators;
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{

		const char *sql = "select operator_id, name from Operators where permission != 'admin'";

		char** pResult;
		int nRow;
		int nCol;
		rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, NULL);
		if (rc == SQLITE_OK)
		{
			if (nRow >= 1)
			{
				for (int i = nCol; i < (nRow + 1) * nCol - 1; i = i + 2)
				{
					operators[pResult[i]] = pResult[i + 1];
				}
			}
		}
		sqlite3_free_table(pResult);
	}
	sqlite3_close(db);

	return operators;
}

bool CLandmarkEditDlg::DB_DelOperotors(std::vector<string> ops)
{
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		for (int i = 0; i < ops.size(); ++i)
		{
			string sql = "delete from Operators where operator_id = " + ops[i];
			const char *sql_del_op = sql.c_str();
			rc = sqlite3_exec(db, sql_del_op, NULL, NULL, NULL);
		}
	}
	sqlite3_close(db);

	if (rc == SQLITE_OK)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool CLandmarkEditDlg::DB_ChangeState(CString op_id, CString state)
{
	int rc = sqlite3_open(dbname, &db);
	string op = CStringA(op_id);
	string stat = CStringA(state);

	if (rc == SQLITE_OK)
	{
		string sql = "update Operators set state = '" + stat + "' where operator_id = " + op;
		const char *sql_change_state = sql.c_str();
		rc = sqlite3_exec(db, sql_change_state, NULL, NULL, NULL);
	}
	sqlite3_close(db);

	if (rc == SQLITE_OK)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool CLandmarkEditDlg::DB_UpdateImageOp(int image_id, CString op_id)
{
	int rc = sqlite3_open(dbname, &db);
	string op = CStringA(op_id);
	stringstream ss;
	ss << image_id;
	if (rc == SQLITE_OK)
	{
		string sql = "update Images set operator_id= " + op + " where image_id= " + ss.str();
		const char *sql_change_op = sql.c_str();
		rc = sqlite3_exec(db, sql_change_op, NULL, NULL, NULL);
	}
	ss.str("");
	sqlite3_close(db);

	if (rc == SQLITE_OK)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool CLandmarkEditDlg::DB_WriteData()
{
	int rc;
	ConnectServer();
	rc = sqlite3_open(dbname, &db);
	TRACE(L"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz---%d\n", rc);
	if (rc == SQLITE_OK)
	{
		rc = sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
		char sql_insert_image[256];
		string path_db = PathToDB(imagePath);
		sprintf(sql_insert_image, "insert into Images values(NULL, '%s', '%d', '%d' , date('now'), '%d')", path_db.c_str(), image_width, image_height, _ttoi(op_id));
		rc = sqlite3_exec(db, sql_insert_image, NULL, NULL, NULL);
		rc = sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

		int amount;
		string sql_search = "select image_id from Images where file_path='" + path_db + "'";
		const char *sql_search_image = sql_search.c_str();
		char** pResult;
		int nRow;
		int nCol;
		rc = sqlite3_get_table(db, sql_search_image, &pResult, &nRow, &nCol, NULL);
		if (rc == SQLITE_OK)
		{
			amount = atoi(pResult[nCol]);
		}
		sqlite3_free_table(pResult);

		rc = sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
		map<int, CRect>::iterator it;
		for (it = db_rect.begin(); it != db_rect.end(); ++it)
		{
			int figure = it->first;
			stringstream ss;
			ss << amount;
			string sql = "insert into Figures values(NULL, " + ss.str();
			ss.str("");  ss << db_rect[figure].left;  sql += ", " + ss.str();
			ss.str("");  ss << db_rect[figure].top;   sql += ", " + ss.str();
			ss.str("");  ss << db_rect[figure].Width(); sql += ", " + ss.str();
			ss.str("");  ss << db_rect[figure].Height(); sql += ", " + ss.str();
			ss.str("");  ss << db_upper_rect[figure].left; sql += ", " + ss.str();
			ss.str("");  ss << db_upper_rect[figure].top;  sql += ", " + ss.str();
			ss.str("");  ss << db_upper_rect[figure].Width(); sql += ", " + ss.str();
			ss.str("");  ss << db_upper_rect[figure].Height(); sql += ", " + ss.str();
			ss.str("");  ss << db_lower_rect[figure].left;  sql += ", " + ss.str();
			ss.str("");  ss << db_lower_rect[figure].top;   sql += ", " + ss.str();
			ss.str("");  ss << db_lower_rect[figure].Width(); sql += ", " + ss.str();
			ss.str("");  ss << db_lower_rect[figure].Height(); sql += ", " + ss.str();
			map<string, string, greater<string>> temp = db_pro_values[figure];
			for (int i = 0; i < FieldName.size(); ++i)
			{
				sql += ",'" + temp[FieldName[i]] + "'";
			}
			sql += ")";

			const char *sql_insert = sql.c_str();
			rc = sqlite3_exec(db, sql_insert, NULL, NULL, NULL);
		}
		rc = sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
		TRACE(L"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz---%d\n", rc);
	}
	sqlite3_close(db);

	char buff[7] = "op_end";
	send(sockClient, buff, sizeof(buff), 0);
	closesocket(sockClient);
	WSACleanup();

	if (rc == SQLITE_OK)
	{
		rc = true;
	}
	else
	{
		rc = false;
	}
	return rc;
}

void CLandmarkEditDlg::DB_ReadData(CString imagePath)
{
	ConnectServer();
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		existed_image_id = DB_ImageExisted(imagePath);
		InitRects(existed_image_id);
	}
	sqlite3_close(db);

	char buff[7] = "op_end";
	send(sockClient, buff, sizeof(buff), 0);
	closesocket(sockClient);
}

void CLandmarkEditDlg::DB_UpdateData()
{
	ConnectServer();
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		rc = sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
		stringstream ss;
		for (int i = 0; i < erased_figure.size(); ++i)
		{
			ss.str("");
			ss << erased_figure[i];
			string delete_f = "delete from Figures where figure_id =" + ss.str();
			const char *del = delete_f.c_str();
			rc = sqlite3_exec(db, del, NULL, NULL, NULL);
		}
		
		for (int i = 0; i < added_figure.size(); ++i)
		{
			CRect b = db_rect[added_figure[i]];
			CRect u = db_upper_rect[added_figure[i]];
			CRect l = db_lower_rect[added_figure[i]];
			map<string, string, greater<string>> pro = db_pro_values[added_figure[i]];
			
			ss.str("");
			ss << existed_image_id;
			string sql = "insert into Figures values(NULL, " + ss.str();
			ss.str("");  ss << b.left;  sql += ", " + ss.str();
			ss.str("");  ss << b.top;   sql += ", " + ss.str();
			ss.str("");  ss << b.Width(); sql += ", " + ss.str();
			ss.str("");  ss << b.Height(); sql += ", " + ss.str();
			ss.str("");  ss << u.left; sql += ", " + ss.str();
			ss.str("");  ss << u.top;  sql += ", " + ss.str();
			ss.str("");  ss << u.Width(); sql += ", " + ss.str();
			ss.str("");  ss << u.Height(); sql += ", " + ss.str();
			ss.str("");  ss << l.left;  sql += ", " + ss.str();
			ss.str("");  ss << l.top;   sql += ", " + ss.str();
			ss.str("");  ss << l.Width(); sql += ", " + ss.str();
			ss.str("");  ss << l.Height(); sql += ", " + ss.str();

			map<string, string, greater<string>>::iterator iter;
			for (iter = pro.begin(); iter != pro.end(); ++iter)
			{
				sql += ",'" + iter->second + "'";
			}
			sql += ")";
			const char *sql_insert = sql.c_str();
			rc = sqlite3_exec(db, sql_insert, NULL, NULL, NULL);
		}
		//todo 删除和新增都存在的情况没有考虑
		map<int, CRect>::iterator it;
		for (it = db_rect.begin(); it != db_rect.end(); ++it)
		{
			CRect b = it->second;
			CRect u = db_upper_rect[it->first];
			CRect l = db_lower_rect[it->first];
			map<string, string, greater<string>> pro = db_pro_values[it->first];
				
			if (find(added_figure.begin(), added_figure.end(), it->first) == added_figure.end())
			{
				string sql_update;
				ss.str("");
				ss << b.left;
				sql_update = "update Figures set body_left=" + ss.str();
				ss.str("");

				ss << b.top;
				sql_update += ", body_top=" + ss.str();
				ss.str("");

				ss << b.Width();
				sql_update += ", body_width=" + ss.str();
				ss.str("");

				ss << b.Height();
				sql_update += ", body_height=" + ss.str();
				ss.str("");

				ss << u.left;
				sql_update += ", upper_left=" + ss.str();
				ss.str("");

				ss << u.top;
				sql_update += ", upper_top=" + ss.str();
				ss.str("");

				ss << u.Width();
				sql_update += ", upper_width=" + ss.str();
				ss.str("");

				ss << u.Height();
				sql_update += ", upper_height=" + ss.str();
				ss.str("");

				ss << l.left;
				sql_update += ", lower_left=" + ss.str();
				ss.str("");

				ss << l.top;
				sql_update += ", lower_top=" + ss.str();
				ss.str("");

				ss << l.Width();
				sql_update += ", lower_width=" + ss.str();
				ss.str("");

				ss << l.Height();
				sql_update += ", lower_height=" + ss.str();
				ss.str("");

				map<string, string, greater<string>>::iterator iter;
				for (iter = pro.begin(); iter != pro.end(); ++iter)
				{
					sql_update += ", " + iter->first + "='" + iter->second + "'";
				}

				ss << it->first;
				sql_update += " where figure_id=" + ss.str();
				ss.str("");

				const char *sql = sql_update.c_str();
				rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
			}

		}

		string op = CStringA(op_id);
		ss << existed_image_id;
		string update = "update Images set operator_id= " + op + " where image_id= " + ss.str();
		ss.str("");
		const char *sql_change_op = update.c_str();
		rc = sqlite3_exec(db, sql_change_op, NULL, NULL, NULL);

		rc = sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
	}
	sqlite3_close(db);

	char buff[7] = "op_end";
	send(sockClient, buff, sizeof(buff), 0);
	closesocket(sockClient);
}

std::vector<string> CLandmarkEditDlg::split(string str, string separator)
{
	std::vector<string> result;
	int cutAt;
	while ((cutAt = str.find_first_of(separator)) != str.npos)
	{
		if (cutAt > 0)
		{
			result.push_back(str.substr(0, cutAt));
		}
		str = str.substr(cutAt + 1);
	}
	if (str.length() > 0)
	{
		result.push_back(str);
	}
	return result;
}

string CLandmarkEditDlg::PathToDB(CString imagePath)
{
	string tempPath = (CStringA)imagePath;
	std::vector<string> temp = split(tempPath, "\\");
	string toDB = temp[temp.size() - 2] + "\\" + temp[temp.size() - 1];
	return toDB;
}

void CLandmarkEditDlg::Merge(CString imageName)
{
	string tempPath = (CStringA)imagePath;
	std::vector<string> temp = split(tempPath, "\\");
	string cate = temp[temp.size() - 2];
	CString ct(cate.c_str());

	CString category = save_path + "\\" + ct;
	if (!PathIsDirectory(category))
	{
		CreateDirectory(category, 0);
	}

	CString temp_to = category + "\\" + imageName;
	CString temp_from = basePath + "\\" + imageName;
	bool done = CopyFile(temp_from, temp_to, true);
	if (!done)
	{
		AfxMessageBox(L"图片已存在，无法复制", MB_OK | MB_ICONERROR);
	}
	else
	{
		DeleteFile(temp_from);
	}
}

CString CLandmarkEditDlg::Encrypt(CString S, WORD Key)
{
	CString Result, str;
	int i, j;
	Result = S; // 初始化结果字符串
	for (i = 0; i<S.GetLength(); i++) // 依次对字符串中各字符进行操作
	{
		Result.SetAt(i, S.GetAt(i) ^ (Key >> 8)); // 将密钥移位后与字符异或
		Key = ((BYTE)Result.GetAt(i) + Key)*C1 + C2; // 产生下一个密钥
	}
	S = Result; // 保存结果
	Result.Empty(); // 清除结果
	for (i = 0; i<S.GetLength(); i++) // 对加密结果进行转换
	{
		j = (BYTE)S.GetAt(i); // 提取字符
		// 将字符转换为两个字母保存
		str = "12"; // 设置str长度为2
		str.SetAt(0, 65 + j / 26);//这里将65改大点的数例如256，密文就会变乱码，效果更好，相应的，解密处要改为相同的数
		str.SetAt(1, 65 + j % 26);
		Result += str;
	}
	return Result;
}

CString CLandmarkEditDlg::Decrypt(CString S, WORD Key)
{
	CString Result, str;
	int i, j;

	Result.Empty(); // 清除结果
	for (i = 0; i < S.GetLength() / 2; i++) // 将字符串两个字母一组进行处理
	{
		j = ((BYTE)S.GetAt(2 * i) - 65) * 26;//相应的，解密处要改为相同的数
		j += (BYTE)S.GetAt(2 * i + 1) - 65;
		str = "1"; // 设置str长度为1
		str.SetAt(0, j);
		Result += str; // 追加字符，还原字符串
	}
	S = Result; // 保存中间结果
	for (i = 0; i<S.GetLength(); i++) // 依次对字符串中各字符进行操作
	{
		Result.SetAt(i, (BYTE)S.GetAt(i) ^ (Key >> 8)); // 将密钥移位后与字符异或
		Key = ((BYTE)S.GetAt(i) + Key)*C1 + C2; // 产生下一个密钥
	}
	return Result;
}

CString CLandmarkEditDlg::SetSavePath()
{
	TCHAR szFolderPath[MAX_PATH] = { 0 };
	CString strFolderPath = TEXT("");

	BROWSEINFO sInfo;
	::ZeroMemory(&sInfo, sizeof(BROWSEINFO));
	sInfo.pidlRoot = 0;
	sInfo.lpszTitle = _T("选择文件保存路径：");
	sInfo.ulFlags = BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
	sInfo.lpfn = NULL;

	// 显示文件夹选择对话框  
	LPITEMIDLIST lpidlBrowse = ::SHBrowseForFolder(&sInfo);
	if (lpidlBrowse != NULL)
	{
		// 取得文件夹名  
		if (::SHGetPathFromIDList(lpidlBrowse, szFolderPath))
		{
			strFolderPath = szFolderPath;
		}
	}
	if (lpidlBrowse != NULL)
	{
		::CoTaskMemFree(lpidlBrowse);
	}
	return strFolderPath;
}

string CLandmarkEditDlg::GetNearest(cv::Scalar scl, std::map<string, cv::Scalar> color_hvalue)
{
	std::map<string, cv::Scalar>::iterator it;
	string res = color_hvalue.begin()->first;
	cv::Scalar tmp = color_hvalue.begin()->second;
	double litte_gap = sqrt(pow(scl.val[0] - tmp.val[0], 2) + pow(scl.val[1] - tmp.val[1], 2) + pow(scl.val[2] - tmp.val[2], 2));
	for (it = color_hvalue.begin(); it != color_hvalue.end(); ++it)
	{
		cv::Scalar clr = it->second;
		double gap = sqrt(pow(scl.val[0] - clr.val[0], 2) + pow(scl.val[1] - clr.val[1], 2) + pow(scl.val[2] - clr.val[2], 2));
		TRACE("\n------------------------%lf\n", gap);
		if (litte_gap > gap)
		{
			res = it->first;
			litte_gap = gap;
		}
	}
	return res;
}

CString CLandmarkEditDlg::GetRgbValue(CPoint pt)
{
	/**CString res = L"";
	if (pt.x >= LEFT_BEGIN&&pt.x <= LEFT_BEGIN + IM_WIDTH&&pt.y >= TOP_BEGIN&&pt.y <= TOP_BEGIN + IM_HEIGHT && !imagePath.IsEmpty())
	{
		int x = (pt.x - LEFT_BEGIN)*image_width / IM_WIDTH;
		int y = (pt.y - TOP_BEGIN)*image_height / IM_HEIGHT;
		IplImage *image = cvLoadImage(PicPath, 1);
		CvScalar sclr = cvGet2D(image, y, x);
		cvReleaseImage(&image);
		string intro = GetNearest(sclr, color_scr);
		CString selected = CString(intro.c_str());
		int r = (int)sclr.val[2];
		int g = (int)sclr.val[1];
		int b = (int)sclr.val[0];
		res.Format(L"(r:%d, g:%d, b:%d)", r, g, b);
		res += selected;
	}
	return res;*/
	CString res = L"";
	if (pt.x >= LEFT_BEGIN&&pt.x <= LEFT_BEGIN + IM_WIDTH&&pt.y >= TOP_BEGIN&&pt.y <= TOP_BEGIN + IM_HEIGHT && !imagePath.IsEmpty())
	{
		int x = (pt.x - LEFT_BEGIN)*image_width / IM_WIDTH;
		int y = (pt.y - TOP_BEGIN)*image_height / IM_HEIGHT;
		IplImage *image = cvLoadImage(PicPath);
		cv::Mat rgb(image);
		cv::Mat hls;
		cv::cvtColor(rgb, hls, CV_BGR2HLS);
		cvReleaseImage(&image);
		int H = hls.at<cv::Vec3b>(y, x)[0];
		int L = hls.at<cv::Vec3b>(y, x)[1];
		int S = hls.at<cv::Vec3b>(y, x)[2];
		//change to windows CSL
		float EF = (float)H * 240.0 / 180;
		float LF = (float)L * 240.0 / 255.0;
		float SF = (float)S * 240.0 / 255.0;
		res.Format(L"(E:%lf, S:%lf, L:%lf)", EF, SF, LF);
	}
	return res;
}

void CLandmarkEditDlg::Lock()
{
	if ((m_pMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, L"db_mutex"))== NULL)
	{
		m_pMutex = CreateMutex(NULL, false, L"db_mutex");
		WaitForSingleObject(m_pMutex, INFINITE);
	}
	else
	{
		WaitForSingleObject(m_pMutex, INFINITE);
		int a = 0;
	}
}

void CLandmarkEditDlg::UnLock()
{
	ReleaseMutex(&m_pMutex);
	CloseHandle(m_pMutex);
}

void CLandmarkEditDlg::ConnectServer()
{
	//加载套接字
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		TRACE("Failed to load Winsock");
		return ;
	}

	string txtPath = save_path + "\\ip.txt";
	ifstream infile(txtPath);
	string ip;
	getline(infile,ip);
	infile.close();

	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(2501);
	addrSrv.sin_addr.S_un.S_addr = inet_addr(ip.c_str());

	//创建套接字
	sockClient = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == sockClient){
		TRACE("Socket() error:%d", WSAGetLastError());
		return ;
	}

	//向服务器发出连接请求
	if (connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(addrSrv)) == INVALID_SOCKET){
		printf("Connect failed:%d", WSAGetLastError());
		return ;
	}
	char buff[9] = "op_begin";
	send(sockClient, buff, sizeof(buff), 0);

	int bytesRecv = SOCKET_ERROR;
	char recvbuf[3] = "";
	for (int i = 0; i<(int)strlen(recvbuf); i++)
	{
		recvbuf[i] = '\0';
	}
	while (bytesRecv == SOCKET_ERROR)
	{
		CMessageDlg message;
		message.DoModal();
		bytesRecv = recv(sockClient, recvbuf, sizeof(recvbuf), 0);
	}
}

WCHAR *CLandmarkEditDlg::mbcsToUnicode(const char *zFilename)
{
	int nByte;
	WCHAR *zMbcsFilename;
	int codepage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
	nByte = MultiByteToWideChar(codepage, 0, zFilename, -1, NULL, 0)*sizeof(WCHAR);
	zMbcsFilename = (WCHAR *)malloc(nByte*sizeof(zMbcsFilename[0]));
	if (zMbcsFilename == 0)
	{
		return 0;
	}
	nByte = MultiByteToWideChar(codepage, 0, zFilename, -1, zMbcsFilename, nByte);
	if (nByte == 0)
	{
		free(zMbcsFilename);
		zMbcsFilename = 0;
	}
	return zMbcsFilename;
}

char *CLandmarkEditDlg::unicodeToUtf8(const WCHAR *zWideFilename)
{
	int nByte;
	char *zFilename;
	nByte = WideCharToMultiByte(CP_UTF8, 0, zWideFilename, -1, 0, 0, 0, 0);
	zFilename = (char *)malloc(nByte);
	if (zFilename == 0)
		return 0;
	nByte = WideCharToMultiByte(CP_UTF8, 0, zWideFilename, -1, zFilename, nByte, 0, 0);
	if (nByte == 0)
	{
		free(zFilename);
		zFilename = 0;
	}
	return zFilename;
}