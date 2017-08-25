
// LandmarkEditDlg.cpp : implementation file
//

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
#define IM_WIDTH 500         //固定图片的长宽比例
#define IM_HEIGHT 500

int LEFT_BEGIN;
int TOP_BEGIN = 200;

//画图相关全局变量
CString imagePath;            //当前处理图片的绝对路径
CString basePath = L"";       //当前处理图片的上级路径
CString currentImageName;     //当前处理图片的名称
char *PicPath;                //当前处理图片绝对路径的字符串数组表示
CString save_path = L"";      //数据库文件保存的文件夹，以及处理之后图片的存放路径

int image_width;              //图片的宽度
int image_height;             //图片的高度
CRect MainRect;               //客户区
bool is_move = false;         //鼠标移动标志
CDC dc_mem;                   //内存画布句柄
CDC *pDC = NULL;
CBitmap bitmap;
shape_predictor sp;
bool add_new_face = true;     //自定义人脸标志
CPoint rect_start(0,0);       //自定义人脸矩形框的左上和右下点
CPoint rect_end(0,0);

//数据库相关全局变量
int drag_face = -1;           //当前拖动人脸的id
sqlite3 *db;
char *dbname;
int face_amount;              //当前数据库中face_id的最大值
int image_amount;             //当前数据库中image_id的最大值
int existed_image_id;         //如果当前图片数据在数据库中存在，则获取该图片的image_id
bool edit = false;            //编辑关键点标志位

bool read_from_database = false;
CString op_id;                          //当前用户id
CListCtrl m_listCtrl;                   //查询结果列表
CDateTimeCtrl m_dateStart;              //查询起始日期
CDateTimeCtrl m_dateEnd;                //查询终止日期
bool save_option_done = true;           //是否将当前处理图片剪切到了指定文件夹
std::map<string, string> PropertyValue; //属性与属性值对应map
std::vector<string> textFieldName;      //可编辑字段的字段名
std::vector<string> profileFieldName;   //profile可编辑字段名
std::map<string, std::vector<string>>  landmark_pro_values;        //关键点每个属性对应的所有可选值
std::map<string, std::vector<string>>  profile_pro_values;         //侧脸属性对应的可选值
std::vector<string> keyPoint_fieldName; //保存关键点在数据库中对应的字段名
std::map<string, string> operators;     //用户信息
std::vector<string> to_del_ops;         //保存准备删除的用户id
std::vector<int> erased_faces;
std::vector<int> added_faces;

//关键点对话框相关全局变量
std::vector<CPoint> face_lm;            //当前编辑的人脸关键点坐标
cv::Mat face_image;                     //截取当前编辑的人脸图片
int face_image_left;                    //当前人脸在图片中的起点坐标
int face_image_top;
int pro_dlg_left;
int pro_dlg_top;
SOCKET sockClient;
bool sock = false;               //是否使用socket控制多人数据库操作

bool landmark_task = true;
bool profile_task = false;
std::map<string, std::map<int, CRect>> Faces;
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
	CComboBox m_combobox[10];
	int dlg_width = 320;
	int dlg_height = 300;
	CEditPropertyDlg();
	enum { IDD = IDD_EDIT_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
public:
	void InitWidget();
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
	std::map<string, string>::iterator it;
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
	std::map<string, string>::iterator it;
	int i = 0;
	for (it = PropertyValue.begin(); it != PropertyValue.end(); ++it)
	{
		string caption = it->first;
		CString m_caption = CString(caption.c_str());
		m_text[i].Create(m_caption, WS_CHILD | WS_VISIBLE, CRect(edit_rect.left + 20, edit_rect.top + 5 + i * 40, edit_rect.left + 120, edit_rect.top + i * 40 + 35), this, IDC_MY_TEXT + i);

		m_combobox[i].Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS, 
			CRect(edit_rect.left + 140, edit_rect.top + 5 + i * 40,edit_rect.left + 290, edit_rect.top + i * 40 + 35), this, IDC_MY_COMBO + i);
		std::vector<string> values;
		if (landmark_task)
		{
			values = landmark_pro_values[caption];
		}
		else if (profile_task)
		{
			values = profile_pro_values[caption];
		}
		if (it->second != "")
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
		else
		{
			for (int j = 0; j < values.size(); ++j)
			{
				m_combobox[i].AddString(CString(values[j].c_str()));
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
	CComboBox m_combobox;
	int selected;
	CString property_name;
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

	m_combobox.AddString(L"landmark属性");
	m_combobox.AddString(L"profile属性");
	m_combobox.SetCurSel(0);
	selected = 0;

	return true;
}

void CAddPropertyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PRO_NAME, property_name);
	DDX_Text(pDX, IDC_PRO_VALUE, property_value);
	DDX_Control(pDX, IDC_COMBO1, m_combobox);
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
END_MESSAGE_MAP()

/*
CEditLmDlg 编辑关键点对话框
*/
class CEditLmDlg : public CDialogEx
{
public:
	CDC *edit_dc;
	CDC edit_mem;
	CImage image;
	int im_w;
	int im_h;
	CRect edit_rect;
	bool moving;
	int move_num;
	int face_width = 250;
	int face_height = 300;
	CEditLmDlg();
	enum{ IDD = IDD_EDIT_LM };

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
	DECLARE_MESSAGE_MAP();

public:
	void MatToCImage(cv::Mat mat, CImage &cImage);
	afx_msg void OnPaint();
	void DrawOnBuffer();
	void DrawEclipse(int x, int y, int r, COLORREF color); //在内存绘制圆形
	void DrawLine(CPoint X, CPoint Y); //在内存绘制线段
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	void ChangeSize(UINT nID, int x, int y);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

CEditLmDlg::CEditLmDlg() : CDialogEx(CEditLmDlg::IDD)
{}

BOOL CEditLmDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	edit_dc = GetDC();
	MatToCImage(face_image, image);

	im_w = image.GetWidth();
	im_h = image.GetHeight();

	SetWindowPos(&wndTopMost, LEFT_BEGIN, 40, face_width + 20, face_height + 100, NULL);
	GetClientRect(&edit_rect);

	return true;
}

void CEditLmDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CEditLmDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
		return TRUE;
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
		return TRUE;
	else
		return CDialog::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CEditLmDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
END_MESSAGE_MAP()

void CEditLmDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	for (int i = 0; i < face_lm.size(); ++i)
	{
		CPoint x((face_lm[i].x - face_image_left) * face_width / im_w, (face_lm[i].y - face_image_top) * face_height / im_h);
		if (4 >= sqrt((x.x - point.x)*(x.x - point.x) + (x.y - point.y) * (x.y - point.y)))
		{
			move_num = i;
			moving = true;
			break;
		}
	}
	CDialogEx::OnLButtonDown(nFlags, point);
}

void CEditLmDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (moving && point.x >= 0 && point.y >= 0 && point.x <= face_width && point.y <= face_height)
	{
		face_lm[move_num].x = point.x * im_w / face_width + face_image_left;
		face_lm[move_num].y = point.y * im_h / face_height + face_image_top;

		Invalidate();
		moving = false;
	}
	CDialogEx::OnLButtonUp(nFlags, point);
}

void CEditLmDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (moving && point.x >= 0 && point.y >= 0 && point.x <= face_width && point.y <= face_height)
	{
		face_lm[move_num].x = point.x * im_w / face_width + face_image_left;
		face_lm[move_num].y = point.y * im_h / face_height + face_image_top;

		Invalidate();
	}
	CDialogEx::OnMouseMove(nFlags, point);
}

void CEditLmDlg::OnPaint()
{
	CDialogEx::OnPaint();
	DrawOnBuffer();
}

void CEditLmDlg::DrawOnBuffer()
{
    CRgn rgn;
	rgn.CreateRectRgn(0, 0, face_width, face_height);
	edit_dc->SelectClipRgn(&rgn);

	edit_mem.CreateCompatibleDC(edit_dc);
	bitmap.CreateCompatibleBitmap(edit_dc, edit_rect.Width(), edit_rect.Height());
	CBitmap *pOldBit = edit_mem.SelectObject(&bitmap);
	edit_mem.FillSolidRect(edit_rect, edit_dc->GetBkColor());

	edit_mem.SetStretchBltMode(HALFTONE);
	CRect rect(0, 0, face_width, face_height);
	image.Draw(edit_mem.m_hDC, rect);

	if (!face_lm.empty())
	{
		for (int i = 0; i < face_lm.size(); ++i)
		{
			DrawEclipse((face_lm[i].x - face_image_left) * face_width / im_w, (face_lm[i].y - face_image_top) * face_height / im_h, 4, RGB(0, 255, 0));
			if (i < face_lm.size() - 1)// && i != 16 && i != 21 && i != 26 && i != 35 && i != 41 && i != 47)
			{
				CPoint x((face_lm[i].x - face_image_left) * face_width / im_w, (face_lm[i].y - face_image_top) * face_height / im_h);
				CPoint y((face_lm[i + 1].x - face_image_left) * face_width / im_w, (face_lm[i + 1].y - face_image_top) * face_height / im_h);
				DrawLine(x, y);
			}
		}
	}

	edit_dc->BitBlt(0, 0, edit_rect.Width(), edit_rect.Height(), &edit_mem, 0, 0, SRCCOPY);

	edit_dc->SelectClipRgn(NULL);
	edit_mem.DeleteDC();
	bitmap.DeleteObject();
}

BOOL CEditLmDlg::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CEditLmDlg::DrawEclipse(int x, int y, int r, COLORREF color)
{
	CBrush brush, *oldbrush;
	brush.CreateSolidBrush(color);
	oldbrush = edit_mem.SelectObject(&brush);
	edit_mem.Ellipse(x - r, y - r, x + r, y + r);
	edit_mem.SelectObject(oldbrush);
}

void CEditLmDlg::DrawLine(CPoint X, CPoint Y)
{
	CPen pen(PS_SOLID, 1, RGB(0, 255, 0));
	CPen *pOldPen = edit_mem.SelectObject(&pen);
	edit_mem.MoveTo(X);
	edit_mem.LineTo(Y);
	edit_mem.SelectObject(pOldPen);
}

void CEditLmDlg::ChangeSize(UINT nID, int x, int y)
{
	CWnd *pWnd;
	pWnd = GetDlgItem(nID);
	if (pWnd != NULL)
	{
		CRect rec;
		pWnd->GetWindowRect(&rec);
		ScreenToClient(&rec);
		rec.left = rec.left*x / edit_rect.Width();
		rec.top = rec.top*y / edit_rect.Height();
		rec.bottom = rec.bottom*y / edit_rect.Height();
		rec.right = rec.right*x / edit_rect.Width();
		pWnd->ShowWindow(FALSE);
		pWnd->ShowWindow(TRUE);
		pWnd->MoveWindow(rec);
	}
}

void CEditLmDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if (nType != SIZE_MINIMIZED)
	{
		ChangeSize(IDOK, cx, cy);
		ChangeSize(IDCANCEL, cx, cy);
		GetClientRect(&edit_rect);
	}
}

void CEditLmDlg::MatToCImage(cv::Mat mat, CImage &cImage)
{
	if (0 == mat.total())
	{
		return;
	}
	int nChannels = mat.channels();
	if ((1 != nChannels) && (3 != nChannels))
	{
		return;
	}
	int nWidth = mat.cols;
	int nHeight = mat.rows;

	//重建cimage
	cImage.Destroy();
	cImage.Create(nWidth, nHeight, 8 * nChannels);

	//拷贝数据
	uchar* pucRow;									//指向数据区的行指针
	uchar* pucImage = (uchar*)cImage.GetBits();		//指向数据区的指针
	int nStep = cImage.GetPitch();					//每行的字节数,注意这个返回值有正有负

	if (1 == nChannels)								//对于单通道的图像需要初始化调色板
	{
		RGBQUAD* rgbquadColorTable;
		int nMaxColors = 256;
		rgbquadColorTable = new RGBQUAD[nMaxColors];
		cImage.GetColorTable(0, nMaxColors, rgbquadColorTable);
		for (int nColor = 0; nColor < nMaxColors; nColor++)
		{
			rgbquadColorTable[nColor].rgbBlue = (uchar)nColor;
			rgbquadColorTable[nColor].rgbGreen = (uchar)nColor;
			rgbquadColorTable[nColor].rgbRed = (uchar)nColor;
		}
		cImage.SetColorTable(0, nMaxColors, rgbquadColorTable);
		delete[]rgbquadColorTable;
	}

	for (int nRow = 0; nRow < nHeight; nRow++)
	{
		pucRow = (mat.ptr<uchar>(nRow));
		for (int nCol = 0; nCol < nWidth; nCol++)
		{
			if (1 == nChannels)
			{
				*(pucImage + nRow * nStep + nCol) = pucRow[nCol];
			}
			else if (3 == nChannels)
			{
				for (int nCha = 0; nCha < 3; nCha++)
				{
					*(pucImage + nRow * nStep + nCol * 3 + nCha) = pucRow[nCol * 3 + nCha];
				}
			}
		}
	}
}


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

// CLandmarkEditDlg dialog
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
	ON_COMMAND(ID_NEW_LANDMARK, &CLandmarkEditDlg::OnNewLandmark)
	ON_COMMAND(ID_OPEN_PIC, &CLandmarkEditDlg::OnOpenPic)
	ON_COMMAND(ID_EDIT_LM, &CLandmarkEditDlg::OnEditLm)
	ON_COMMAND(ID_EDIT_PRO, &CLandmarkEditDlg::OnEditPro)
	ON_COMMAND(ID_DELETE_FACE, &CLandmarkEditDlg::OnDeleteFace)
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
	ON_COMMAND(ID_Landmark_task, &CLandmarkEditDlg::OnLandmarktask)
	ON_COMMAND(ID_profile_task, &CLandmarkEditDlg::Onprofiletask)
	ON_COMMAND(ID_Read_Faces, &CLandmarkEditDlg::OnReadFaces)
	ON_COMMAND(ID_Edit_Profile, &CLandmarkEditDlg::OnEditProfile)
END_MESSAGE_MAP()

// CLandmarkEditDlg message handlers
BOOL CLandmarkEditDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	save_path = SetSavePath();
	if (save_path.IsEmpty())
	{
		EndDialog(0);
		return true;
	}
	CString db_path = save_path + "\\landmark.sqlite";
	string txtPath = save_path + "\\ip.txt";
	CFileFind fileFinder;
	if (fileFinder.FindFile(CString(txtPath.c_str())))
	{
		sock = true;
	}

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
				AfxMessageBox(L"密码错误或该用户已登录！", MB_OK | MB_ICONERROR);
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

	SetWindowText(L"LandMark编辑器");

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

	SetDlgItemText(IDC_TEXT01, L"当前用户ID为：" + op_id);

	CRect save_rect;
	GetDlgItem(IDC_SAVE)->GetWindowRect(&save_rect);
	ScreenToClient(&save_rect);
	GetDlgItem(IDC_SAVE)->MoveWindow(LEFT_BEGIN + 10, 950, save_rect.Width(), save_rect.Height(), TRUE);

	CRect next_rect;
	GetDlgItem(IDC_NEXT_PIC)->GetWindowRect(&next_rect);
	ScreenToClient(&next_rect);
	GetDlgItem(IDC_NEXT_PIC)->MoveWindow(LEFT_BEGIN + 100, 950, next_rect.Width(), next_rect.Height(), TRUE);

	CRect del_rect;
	GetDlgItem(IDC_DEL_PIC)->GetWindowRect(&del_rect);
	ScreenToClient(&del_rect);
	GetDlgItem(IDC_DEL_PIC)->MoveWindow(LEFT_BEGIN + 190, 950, del_rect.Width(), del_rect.Height(), TRUE);

	CRect image_rect;
	GetDlgItem(IDC_IMAGE_NAME)->GetWindowRect(&image_rect);
	ScreenToClient(&image_rect);
	GetDlgItem(IDC_IMAGE_NAME)->MoveWindow(LEFT_BEGIN + 10, 10, image_rect.Width(), image_rect.Height(), TRUE);

	sp = GetSP();

	string res = DB_GetFacesCreateSql();
	keyPoint_fieldName = DB_GetKeyPointFieldName(res);

	GetProNameAndValues(textFieldName, profileFieldName, landmark_pro_values, profile_pro_values);

	return TRUE;  // return TRUE  unless you set the focus to a control
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
	if (add_new_face && point.x >= LEFT_BEGIN && point.x <= LEFT_BEGIN + IM_WIDTH && point.y >= TOP_BEGIN && point.y <= TOP_BEGIN + IM_HEIGHT)
	{
		is_move = true;
		rect_start = point;
	}
	CDialogEx::OnLButtonDown(nFlags, point);
}

void CLandmarkEditDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (is_move && add_new_face && point.x >= LEFT_BEGIN && point.x <= LEFT_BEGIN + IM_WIDTH && point.y >= TOP_BEGIN && point.y <= TOP_BEGIN + IM_HEIGHT)
	{
		rect_end.x = point.x;
		rect_end.y = rect_start.y + point.x - rect_start.x;
		is_move = false;
		Invalidate();
	}
	CDialogEx::OnLButtonUp(nFlags, point);
}

void CLandmarkEditDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	int face = GetEditFaceId(point);
	if (face != 0)
	{
		drag_face = face;

		pro_dlg_left = point.x;
		pro_dlg_top = point.y;

		CMenu rmenu;
		rmenu.LoadMenuW(IDR_EDIT_MENU);
		CMenu *popUp = rmenu.GetSubMenu(0);
		ClientToScreen(&point);
		popUp->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
	}

	if (point.x >= rect_start.x && point.x <= rect_end.x && point.y >= rect_start.y && point.y <= rect_end.y)
	{
		CMenu rmenu;
		rmenu.LoadMenuW(IDR_RMENU);
		CMenu *popUp = rmenu.GetSubMenu(0);
		ClientToScreen(&point);
		popUp->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
	}
	CDialogEx::OnRButtonDown(nFlags, point);
}

void CLandmarkEditDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	CDialogEx::OnRButtonUp(nFlags, point);
}

void CLandmarkEditDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (is_move && add_new_face && MK_LBUTTON == nFlags && point.x >= LEFT_BEGIN && point.x <= LEFT_BEGIN + IM_WIDTH && point.y >= TOP_BEGIN && point.y <= TOP_BEGIN + IM_HEIGHT)
	{
		rect_end.x = point.x;
		rect_end.y = rect_start.y + point.x - rect_start.x;
		Invalidate();
	}
	CDialogEx::OnMouseMove(nFlags, point);
}

void CLandmarkEditDlg::OnNewLandmark()
{
	std::vector<CPoint> new_face = GetLandmark(rect_start, rect_end);
	CPoint s((rect_start.x - LEFT_BEGIN) * image_width / IM_WIDTH, (rect_start.y - TOP_BEGIN) * image_height / IM_HEIGHT),
		e((rect_end.x - LEFT_BEGIN) * image_width / IM_WIDTH, (rect_end.y - TOP_BEGIN) * image_height / IM_HEIGHT);

	if (db_landmark.empty())
	{
		db_landmark[1] = new_face;
		added_faces.push_back(1);
		db_rect[1] = DB_GetRect(new_face);
	}
	else
	{
		std::map<int, std::vector<CPoint>>::reverse_iterator it = db_landmark.rbegin();
		int tp = it->first + 1;
		db_landmark[tp] = new_face;
		added_faces.push_back(tp);
		db_rect[tp] = DB_GetRect(new_face);
	}
	
	rect_start = rect_end = CPoint(0,0);
	Invalidate();
}

void CLandmarkEditDlg::OnOpenPic()
{
	CFileDialog selectPic(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, L"jpg(*.jpg)|*.jpg|png(*.png)|*.png||");
	if (IDOK == selectPic.DoModal())
	{
		read_from_database = false;

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

		if (!db_landmark.empty())
		{
			db_landmark.clear();
		}
		if (!db_rect.empty())
		{
			db_rect.clear();
		}
		if (!erased_faces.empty())
		{
			erased_faces.clear();
		}
		if (!added_faces.empty())
		{
			added_faces.clear();
		}
		if (!db_pro_values.empty())
		{
			db_pro_values.clear();
		}

		ShowImage(imagePath);
		if (!profile_task && landmark_task){
			GetInitialLandmark();
		}
		//string current_image = (CStringA)currentImageName;
		//db_rect = Faces[current_image];
		
		DrawOnBuffer();

		if (!profile_task && landmark_task){
			add_new_face = true;
		}
		save_option_done = false;
	}
}

void CLandmarkEditDlg::OnEditLm()
{
	if (!profile_task && landmark_task)
	{
		add_new_face = false;
		edit = true;

		Invalidate();

		INT_PTR nRes;
		CEditLmDlg edit_lm;
		face_lm = db_landmark[drag_face];

		IplImage *temp_image = cvLoadImage(PicPath);
		cv::Mat image(temp_image);
		cv::Rect rect(db_rect[drag_face].left, db_rect[drag_face].top, db_rect[drag_face].Width(), db_rect[drag_face].Height());
		face_image = image(rect);

		face_image_left = db_rect[drag_face].left;
		face_image_top = db_rect[drag_face].top;

		nRes = edit_lm.DoModal();
		if (nRes == IDOK)
		{
			db_landmark[drag_face] = face_lm;
			db_rect[drag_face] = DB_GetRect(face_lm);
			//DB_UpdateOneFace(drag_face, face_lm);
			//DB_UpdateImageOp(existed_image_id, op_id);

			face_lm.clear();
			cvReleaseImage(&temp_image);

			drag_face = -1;
			edit = false;
			add_new_face = true;
			Invalidate();
		}
		else
		{
			face_lm.clear();
			drag_face = -1;
			edit = false;
			add_new_face = true;
			Invalidate();
		}
	}
}

void CLandmarkEditDlg::OnEditPro()
{
	if (db_pro_values.size() != 0)
	{
		if (!profile_task && landmark_task)
		{
			INT_PTR nRes;
			CEditPropertyDlg properties;

			if (db_pro_values.find(drag_face) == db_pro_values.end())
			{
				std::map<string, string> tmp;
				for (int i = 0; i < textFieldName.size(); ++i)
				{
					tmp[textFieldName[i]] = "";
				}
				db_pro_values[drag_face] = tmp;
			}
			PropertyValue = db_pro_values[drag_face];
			nRes = properties.DoModal();
			if (nRes == IDOK)
			{
				db_pro_values[drag_face] = PropertyValue;
				drag_face = -1;
			}
		}
	}
}

void CLandmarkEditDlg::OnEditProfile()
{
	if (profile_task && !landmark_task)
	{
		INT_PTR nRes;
		CEditPropertyDlg properties;

		if (db_pro_values.find(drag_face) == db_pro_values.end())
		{
			std::map<string, string> tmp;
			for (int i = 0; i < profileFieldName.size(); ++i)
			{
				tmp[profileFieldName[i]] = "";
			}
			db_pro_values[drag_face] = tmp;
		}
		PropertyValue = db_pro_values[drag_face];
		nRes = properties.DoModal();
		if (nRes == IDOK)
		{
			db_pro_values[drag_face] = PropertyValue;
			drag_face = -1;
		}
	}
}

void CLandmarkEditDlg::OnDeleteFace()
{
	erased_faces.push_back(drag_face);
	db_landmark.erase(drag_face);
	db_rect.erase(drag_face);
	if (db_pro_values.find(drag_face)!=db_pro_values.end())
	{
		db_pro_values.erase(drag_face);
	}
	Invalidate();
}

void CLandmarkEditDlg::OnBnClickedNextPic()
{
	if (!read_from_database)
	{
		if (!save_option_done)
		{
			AfxMessageBox(L"当前图片未保存!!!", MB_OK | MB_ICONERROR);
		}
		else
		{
			Images = GetRemainingImages(basePath);
			if (!Images.empty())
			{
				currentImageName = Images[0];
				imagePath = basePath + "\\" + currentImageName;
				SetDlgItemText(IDC_IMAGE_NAME, currentImageName);

				int len = WideCharToMultiByte(CP_ACP, 0, imagePath, -1, NULL, 0, NULL, NULL);
				PicPath = new char[len + 1];
				WideCharToMultiByte(CP_ACP, 0, imagePath, -1, PicPath, len, NULL, NULL);

				if (!db_landmark.empty())
				{
					db_landmark.clear();
				}
				if (!db_rect.empty())
				{
					db_rect.clear();
				}
				if (!erased_faces.empty())
				{
					erased_faces.clear();
				}
				if (!added_faces.empty())
				{
					added_faces.clear();
				}
				if (!db_pro_values.empty())
				{
					db_pro_values.clear();
				}
				save_option_done = false;

				ShowImage(imagePath);
				if (!profile_task && landmark_task){
					GetInitialLandmark();
				}
				//string current_image = (CStringA)currentImageName;
				//db_rect = Faces[current_image];

				DrawOnBuffer();
				if (!profile_task && landmark_task){
					add_new_face = true;
				}
			}
			else
			{
				AfxMessageBox(L"当前文件夹没有未处理图片", MB_OK | MB_ICONINFORMATION);
			}
		}	
	}
}

void CLandmarkEditDlg::OnBnClickedSearch()
{
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
}

void CLandmarkEditDlg::OnNMClickList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	int nIndex = pNMListView->iItem;
	
	if (nIndex >= 0)
	{
		read_from_database = true;
		save_option_done = false;
		CRect rect(LEFT_BEGIN, TOP_BEGIN, LEFT_BEGIN + IM_WIDTH, TOP_BEGIN + IM_HEIGHT);
		pDC->FillSolidRect(&rect, pDC->GetBkColor());

		CString file_path = m_listCtrl.GetItemText(nIndex, 0);
		SetDlgItemText(IDC_IMAGE_NAME, file_path);

		if (!db_landmark.empty())
		{
			db_landmark.clear();
		}
		if (!db_rect.empty())
		{
			db_rect.clear();
		}
		if (!erased_faces.empty())
		{
			erased_faces.clear();
		}
		if (!added_faces.empty())
		{
			added_faces.clear();
		}
		if (!db_pro_values.empty())
		{
			db_pro_values.clear();
		}

		imagePath = save_path + "\\" + file_path;

		int len = WideCharToMultiByte(CP_ACP, 0, imagePath, -1, NULL, 0, NULL, NULL);
		PicPath = new char[len + 1];
		WideCharToMultiByte(CP_ACP, 0, imagePath, -1, PicPath, len, NULL, NULL);

		CFileFind fileFinder;
		if (fileFinder.FindFile(imagePath))
		{
			add_new_face = true;
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
	/*
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
	*/
}

void CLandmarkEditDlg::OnExit()
{
	if (!save_option_done)
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
	else{
		DB_ChangeState(op_id, L"logout");
		EndDialog(0);
	}
}

void CLandmarkEditDlg::OnClose()
{
	if (!save_option_done)
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
	else{
		DB_ChangeState(op_id, L"logout");
		EndDialog(0);
	}
}

void CLandmarkEditDlg::OnChangePwd()
{
	/*
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
	*/
}

void CLandmarkEditDlg::OnAddPro()
{
	/*
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
			DB_AddProperty(add_pro.property_name);
			string new_pro_name = CStringA(add_pro.property_name);
			string new_pro_value = CStringA(add_pro.property_value);
			string cfgPath = save_path + "\\pro_values.ini";
			ofstream outfile(cfgPath, ios::app);
			outfile << add_pro.selected << " " << new_pro_name << ":" << new_pro_value << endl;
			outfile.close();
			if (add_pro.selected == 0)
			{
				textFieldName.push_back(new_pro_name);
				std::vector<string> new_pro_values = split(new_pro_value, ",");
				landmark_pro_values[new_pro_name] = new_pro_values;
			}
			else
			{
				profileFieldName.push_back(new_pro_name);
				std::vector<string> new_pro_values = split(new_pro_value, ",");
				profile_pro_values[new_pro_name] = new_pro_values;
			}
		}
	}
	*/
}

void CLandmarkEditDlg::OnBnClickedDelPic()
{
	/*
	if (!DB_IsAdmin(op_id))
	{
		AfxMessageBox(L"您没有权限删除图片", MB_OK | MB_ICONERROR);
	}
	else
	{
		if (!imagePath.IsEmpty())
		{
			if (AfxMessageBox(L"该图片相关信息以及图片文件都将被删除！！", MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
			{
				if (DB_DelCurrentImage(existed_image_id))
				{
					if (!db_landmark.empty())
					{
						db_landmark.clear();
					}
					if (!db_rect.empty())
					{
						db_rect.clear();
					}
					if (!erased_faces.empty())
					{
						erased_faces.clear();
					}
					if (!added_faces.empty())
					{
						added_faces.clear();
					}
					if (!db_pro_values.empty())
					{
						db_pro_values.clear();
					}
					save_option_done = true;
					DeleteFile(imagePath);
					Invalidate();
				}
			}
		}
	}*/
}

void CLandmarkEditDlg::OnDelOperator()
{
	/*
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
	*/
}

void CLandmarkEditDlg::OnLandmarktask()
{
	/*
	if (!profile_task){
		landmark_task = true;
		sp = GetSP();
	}
	*/

}

void CLandmarkEditDlg::Onprofiletask()
{
	/*
	if (!landmark_task){
		profile_task = true;
	}
	*/
}

void CLandmarkEditDlg::OnReadFaces()
{
	if (profile_task){
		CFileDialog readLabel(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, L"txt(*.txt)|*.txt||");
		if (IDOK == readLabel.DoModal())
		{
			string rect_txt = (CStringA)readLabel.GetPathName();
			ifstream label_file(rect_txt);
			string img_path;
			while (getline(label_file, img_path))
			{
				string faceCount;
				getline(label_file, faceCount);
				std::map<int, CRect> rects;
				for (int i = 0; i < stoi(faceCount); ++i)
				{
					string rect;
					getline(label_file, rect);
					std::vector<string> coords = split(rect, " ");
					rects[i+1] = CRect(stof(coords[0]), stof(coords[1]), stof(coords[0]) + stof(coords[2]), stof(coords[1]) + stof(coords[3]));
				}
				Faces[img_path] = rects;
			}
		}
	}
}

shape_predictor CLandmarkEditDlg::GetSP()
{
	std::string shape_predictor_path = "shape_predictor_68_face_landmarks.dat";
	frontal_face_detector detector = get_frontal_face_detector();
	shape_predictor sp;
	deserialize(shape_predictor_path) >> sp;

	return sp;
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

void CLandmarkEditDlg::GetInitialLandmark()
{
	frontal_face_detector detector = get_frontal_face_detector();
	array2d<rgb_pixel> img;
	load_image(img, PicPath);

	std::vector<dlib::rectangle> dets = detector(img);

	for (unsigned long j = 0; j < dets.size(); ++j)
	{
		CRect rect(dets[j].left(), dets[j].top(), dets[j].right(), dets[j].bottom());
		std::vector<CPoint> key_point;
		full_object_detection shape = sp(img, dets[j]);
		for (unsigned long i = 0; i < shape.num_parts(); i++)
		{
			point pt = shape.part(i);
			int x = pt.x();
			int y = pt.y();
			key_point.push_back(CPoint(x, y));
		}
		db_landmark[j + 1] = key_point;
		db_rect[j + 1] = DB_GetRect(key_point);
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

	if (add_new_face)
	{
		DrawRectangle(rect_start, rect_end);
	}

	if (!db_landmark.empty())
	{
		std::map<int, std::vector<CPoint>>::iterator it;
		for (it = db_landmark.begin(); it != db_landmark.end(); ++it)
		{
			std::vector<CPoint> points = it->second;
			for (int i = 0; i < points.size(); ++i)
			{
				if (drag_face == it->first && edit)
				{
					DrawEclipse(LEFT_BEGIN + IM_WIDTH * points[i].x / image_width, TOP_BEGIN + IM_HEIGHT * points[i].y / image_height, 4, RGB(0, 255, 0));
				}
				else
				{
					DrawEclipse(LEFT_BEGIN + IM_WIDTH * points[i].x / image_width, TOP_BEGIN + IM_HEIGHT * points[i].y / image_height, 4, RGB(255, 255, 255));
				}
				if (i < points.size() - 1) //&& i != 16 && i != 21 && i != 26 && i != 35 && i != 41 && i != 47)
				{
					CPoint x(LEFT_BEGIN + IM_WIDTH * points[i].x / image_width, TOP_BEGIN + IM_HEIGHT * points[i].y / image_height);
					CPoint y(LEFT_BEGIN + IM_WIDTH * points[i + 1].x / image_width, TOP_BEGIN + IM_HEIGHT * points[i + 1].y / image_height);
					DrawLine(x, y);
				}
			}
		}
	}
	/*
	for (int i = 0; i < db_rect.size(); i++)
	{
		CPoint x(LEFT_BEGIN + IM_WIDTH * db_rect[i].left / image_width, TOP_BEGIN + IM_HEIGHT * db_rect[i].top / image_height);
		CPoint y(LEFT_BEGIN + IM_WIDTH * db_rect[i].right / image_width, TOP_BEGIN + IM_HEIGHT * db_rect[i].bottom / image_height);
		DrawRectangle(x, y);
	}
	*/

	pDC->BitBlt(0, 0, MainRect.Width(), MainRect.Height(), &dc_mem, 0, 0, SRCCOPY);

	pDC->SelectClipRgn(NULL);
	dc_mem.DeleteDC();
	bitmap.DeleteObject();
	image.Destroy();
}

void CLandmarkEditDlg::DrawEclipse(int x, int y, int r, COLORREF color)
{
	CBrush brush, *oldbrush;
	brush.CreateSolidBrush(color);
	oldbrush = dc_mem.SelectObject(&brush);
	dc_mem.Ellipse(x - r, y - r, x + r, y + r);
	dc_mem.SelectObject(oldbrush);
}

void CLandmarkEditDlg::DrawLine(CPoint X, CPoint Y)
{
	CPen pen(PS_SOLID, 1, RGB(0, 255, 0));
	CPen *pOldPen = dc_mem.SelectObject(&pen);
	dc_mem.MoveTo(X);
	dc_mem.LineTo(Y);
	dc_mem.SelectObject(pOldPen);
}

void CLandmarkEditDlg::DrawRectangle(CPoint X, CPoint Y)
{
	CPen pen(PS_SOLID, 1, RGB(0, 255, 0));
	CPen *pOldPen = dc_mem.SelectObject(&pen);

	CBrush *pBrush = CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH));
	CBrush *pOldBrush = dc_mem.SelectObject(pBrush);
	dc_mem.Rectangle(CRect(X, Y));

	dc_mem.SelectObject(pOldPen);
	dc_mem.SelectObject(pOldBrush);
}

std::vector<CPoint> CLandmarkEditDlg::GetLandmark(CPoint start, CPoint end)
{
	array2d<rgb_pixel> img;
	load_image(img, PicPath);

	point s((start.x - LEFT_BEGIN) * image_width / IM_WIDTH, (start.y - TOP_BEGIN) * image_height / IM_HEIGHT), 
		e((end.x - LEFT_BEGIN) * image_width / IM_WIDTH, (end.y - TOP_BEGIN) * image_height / IM_HEIGHT);
	
	dlib::rectangle rect(s, e);
	full_object_detection shape = sp(img, rect);
	std::vector<CPoint> key_point;

	for (unsigned long i = 0; i<shape.num_parts(); i++)
	{
		point pt = shape.part(i);
		key_point.push_back(CPoint(pt.x(), pt.y()));
	}

	return key_point;
}

int CLandmarkEditDlg::GetEditFaceId(CPoint point)//TODO 更改代码 根据point是否在矩形框中找到人脸ID
{
	std::map<int, CRect>::iterator it;
	for (it = db_rect.begin(); it != db_rect.end(); ++it)
	{
		CRect rect = it->second;
		int tmp_x = (point.x - LEFT_BEGIN) * image_width / IM_WIDTH;
		int tmp_y = (point.y - TOP_BEGIN) * image_height / IM_HEIGHT;
		if (tmp_x >= rect.left && tmp_x <= rect.right && tmp_y >= rect.top && tmp_y <= rect.bottom)
		{
			return it->first;
		}
	}
	return 0;
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

void CLandmarkEditDlg::SaveOption()
{
	if (!save_option_done)
	{
		if (db_pro_values.size() != 0 && db_pro_values.size() < db_landmark.size())
		{
			AfxMessageBox(L"未标注人脸属性", MB_OK | MB_ICONERROR);
		}
		else
		{
			int flag = 1;
			std::map<int, std::map<string, string>>::iterator it;
			for (it = db_pro_values.begin(); it != db_pro_values.end(); ++it)
			{
				std::map<string, string> pro = it->second;
				std::map<string, string>::iterator iter;
				for (iter = pro.begin(); iter != pro.end(); ++iter)
				{
					if (iter->second.size() == 0)
					{
						AfxMessageBox(L"未标注人脸属性", MB_OK | MB_ICONERROR);
						flag = 0;
						break;
					}
				}
			}
			if (flag == 1)
			{
				if (!imagePath.IsEmpty())
				{
					if (!read_from_database)
					{
						CRect rect(LEFT_BEGIN, TOP_BEGIN, LEFT_BEGIN + IM_WIDTH, TOP_BEGIN + IM_HEIGHT);
						pDC->FillSolidRect(&rect, pDC->GetBkColor());

						DB_WriteData();
						Merge(currentImageName);
						save_option_done = true;
						add_new_face = false;

						if (!db_landmark.empty())
						{
							db_landmark.clear();
						}
						if (!db_rect.empty())
						{
							db_rect.clear();
						}
						if (!erased_faces.empty())
						{
							erased_faces.clear();
						}
						if (!added_faces.empty())
						{
							added_faces.clear();
						}
						if (!db_pro_values.empty())
						{
							db_pro_values.clear();
						}
					}
					else
					{
						DB_UpdateData();
						save_option_done = true;
						add_new_face = false;
					}
				}
			}
		}
	}

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

std::vector<string> CLandmarkEditDlg::DB_GetKeyPointFieldName(string res)
{
	std::vector<string> r = split(res, ",");
	std::vector<string> keyField;
	for (int i = 2; i < r.size(); ++i)
	{
		std::vector<string> tp = split(r[i], "\"");
		if (tp.size() == 2)
		{
			keyField.push_back(tp[0]);
		}
		else if (tp.size() == 3)
		{
			keyField.push_back(tp[1]);
		}
	}
	/*
	ofstream out("locateField.txt");
	for (int i = 0; i < keyField.size();++i)
	{
		out << keyField[i] << endl;
	}
	out.close();
	*/
	return keyField;
}

std::vector<int> CLandmarkEditDlg::DB_GetFaceID(int image_id)
{
	std::vector<int> faces;
	stringstream ss;
	string sql_faces;
	ss << image_id;
	sql_faces = "select face_id from Faces where image_id = " + ss.str();
	ss.str("");
	const char *sql = sql_faces.c_str();

	char** pResult;
	int nRow;
	int nCol;
	int rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, NULL);
	if (rc == SQLITE_OK)
	{
		for (int i = nCol; i < (nRow + 1)*nCol; ++i)
		{
			faces.push_back(atoi(pResult[i]));
		}
	}
	sqlite3_free_table(pResult);
	return faces;
}

std::vector<CPoint> CLandmarkEditDlg::DB_GetPoints(int face_id)
{
	std::vector<CPoint> points;
	stringstream ss;
	string sql_p;
	ss << face_id;
	sql_p = "select " + keyPoint_fieldName[0];

	for (int i = 1; i < keyPoint_fieldName.size(); ++i)
	{
		sql_p += "," + keyPoint_fieldName[i];
	}
	sql_p += " from Faces where face_id = " + ss.str();
	ss.str("");
		
	const char *sql = sql_p.c_str();

	char** pResult;
	int nRow;
	int nCol;
	int rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, NULL);
	if (rc == SQLITE_OK)
	{
		if (nRow >= 1)
		{
			for (int i = nCol; i < (nRow + 1)*nCol - 1; i += 2)
			{
				CPoint pt;
				pt.x = atoi(pResult[i]);
				pt.y = atoi(pResult[i + 1]);
				points.push_back(pt);
			}
		}
	}
	sqlite3_free_table(pResult);
	return points;
}

CRect CLandmarkEditDlg::DB_GetRect(std::vector<CPoint> points)
{
	int l, r, t, b;
	if (!points.empty())
	{
		l = r = points[0].x;
		t = b = points[0].y;
		for (int i = 1; i < points.size(); ++i)
		{
			if (points[i].x < l)
			{
				l = points[i].x;
			}
			if (points[i].x > r)
			{
				r = points[i].x;
			}
			if (points[i].y < t)
			{
				t = points[i].y;
			}
			if (points[i].y > b)
			{
				b = points[i].y;
			}
		}
	}
	CRect rect(l-8, t-8, r+8, b+8);
	return rect;
}

std::vector<CString> CLandmarkEditDlg::DB_GetImagePath(CString event_date_start, 
	                                                   CString event_date_end, CString operator_id)
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
				string pa = pResult[nCol];
				string stat = pResult[nCol + 1];
				string tmp = CStringA(password);
				if (tmp == decode(pa))// && stat == "logout"
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

bool CLandmarkEditDlg::DB_InsertOperator(CString operator_id, CString op_name, CString password, CString permission ,CString state)
{
	string opid = CStringA(operator_id);
	string tmp = (CStringA)password;
	string pa = encode(tmp);
	string pe = CStringA(permission);
	string opn = CStringA(op_name);
	string stat = CStringA(state);

	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		string sql_insert;
		sql_insert = "insert into Operators values ('" + opid + "', '" + pa + "', '" + pe + "','" + opn + "','" + stat +"')";

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
	string old_pwd;
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
				old_pwd = pResult[nCol];
			}
		}
		sqlite3_free_table(pResult);
	}
	sqlite3_close(db);
	string tmp = decode(old_pwd);
	return CString(tmp.c_str());
}

bool CLandmarkEditDlg::DB_SetNewPwd(CString operator_id, CString new_pwd)
{
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		string sql_pwd;
		string tmp = CStringA(new_pwd);
		string tmp1 = CStringA(operator_id);
		sql_pwd = "update Operators set password = '" + encode(tmp) + "' where operator_id = " + tmp1;
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

std::map<string, string> CLandmarkEditDlg::DB_GetProperties(int face_id)
{
	std::map<string, string> properties;
	if (textFieldName.size() != 0)
	{
		stringstream ss;
		string sql_faces;
		ss << face_id;
		sql_faces = "select " + textFieldName[0];

		for (int i = 1; i < textFieldName.size(); ++i)
		{
			sql_faces += "," + textFieldName[i];
		}
		sql_faces += " from Faces where face_id = " + ss.str();

		ss.str("");
		const char *sql = sql_faces.c_str();

		char** pResult;
		int nRow;
		int nCol;
		int rc = sqlite3_get_table(db, sql, &pResult, &nRow, &nCol, NULL);
		if (rc == SQLITE_OK)
		{
			if (nRow >= 1)
			{
				for (int i = 0; i < textFieldName.size(); ++i)
				{
					properties[textFieldName[i]] = pResult[i + nCol];
				}
			}
			else
			{
				for (int i = 0; i < textFieldName.size(); ++i)
				{
					properties[textFieldName[i]] = "";
				}
			}
		}
		sqlite3_free_table(pResult);
	}
	
	return properties;
}

bool CLandmarkEditDlg::DB_AddProperty(CString pro_name)
{
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		string sql = "alter table Faces add column \"" + (CStringA)pro_name + "\" text default ''";

		const char *sql_add_pro = sql.c_str();
		rc = sqlite3_exec(db, sql_add_pro, NULL, NULL, NULL);
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

string CLandmarkEditDlg::DB_GetFacesCreateSql()
{
	string res = "";
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		char *sql = "select sql from sqlite_master where tbl_name = 'Faces' and type = 'table'";
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

		string sql2 = "delete from Faces where image_id = " + ss.str();
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
				for (int i = nCol; i < (nRow + 1) * nCol - 1; i=i+2)
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

void CLandmarkEditDlg::DB_WriteData()
{
	if (sock)
	{
		ConnectServer();
	}
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		rc = sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
		char sql_insert_image[256];
		string path_db = PathToDB(imagePath);
		sprintf(sql_insert_image, "insert into Images values(NULL, '%s', '%d', '%d' , date('now'), '%d')", path_db.c_str(), image_width, image_height, _ttoi(op_id));
		rc = sqlite3_exec(db, sql_insert_image, NULL, NULL, NULL);
		rc = sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

		int image_id;
		char *sql_select = "select seq from sqlite_sequence where name='Images'";
		char** pResult;
		int nRow;
		int nCol;
		rc = sqlite3_get_table(db, sql_select, &pResult, &nRow, &nCol, NULL);
		if (rc == SQLITE_OK)
		{
			image_id = atoi(pResult[nCol]);
		}
		//ofstream log("log.txt", ios::app);
		rc = sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
		std::map<int, std::vector<CPoint>>::iterator it;
		for (it = db_landmark.begin(); it != db_landmark.end(); ++it)
		{
			std::vector<CPoint> points = it->second;
			stringstream ss;
			string sql_insert_faces;
			ss << image_id;
			sql_insert_faces = "insert into Faces values(NULL," + ss.str();
			ss.str("");
			
			for (int i = 0; i < points.size(); ++i)
			{
				ss << points[i].x;
				sql_insert_faces += "," + ss.str();
				ss.str("");
				ss << points[i].y;
				sql_insert_faces += "," + ss.str();
				ss.str("");
			}
		    for (int i = 0; i < textFieldName.size(); ++i)
			{
				sql_insert_faces += ",'" + db_pro_values[it->first][textFieldName[i]]+"'";
			}
			sql_insert_faces += ")";

			
			//log << sql_insert_faces << endl;

			const char *sql = sql_insert_faces.c_str();
			rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
		}
		//log.close();
		rc = sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
	}
	sqlite3_close(db);
	if (sock)
	{
		char buff[7] = "op_end";
		send(sockClient, buff, sizeof(buff), 0);
		closesocket(sockClient);
		WSACleanup();
	}
}

void CLandmarkEditDlg::DB_ReadData(CString imagePath)
{
	if (sock)
	{
		ConnectServer();
	}
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		existed_image_id = DB_ImageExisted(imagePath);

		std::vector<int> faces = DB_GetFaceID(existed_image_id);
		for (int i = 0; i < faces.size(); ++i)
		{
			std::vector<CPoint> tp = DB_GetPoints(faces[i]);
			db_landmark[faces[i]] = tp;
			db_rect[faces[i]] = DB_GetRect(tp);
			std::map<string, string> tp2 = DB_GetProperties(faces[i]);
			if (tp2.size() != 0){
				db_pro_values[faces[i]] = tp2;
			}
		}
	}
	sqlite3_close(db);

	if (sock)
	{
		char buff[7] = "op_end";
		send(sockClient, buff, sizeof(buff), 0);
		closesocket(sockClient);
	}	
}

void CLandmarkEditDlg::DB_UpdateData()
{
	if (sock)
	{
		ConnectServer();
	}
	
	int rc = sqlite3_open(dbname, &db);
	if (rc == SQLITE_OK)
	{
		rc = sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
		stringstream ss;
		for (int i = 0; i < erased_faces.size(); ++i)
		{
			ss.str("");
			ss << erased_faces[i];
			string delete_f = "delete from Faces where face_id =" + ss.str();
			const char *del = delete_f.c_str();
			rc = sqlite3_exec(db, del, NULL, NULL, NULL);
		}

		for (int i = 0; i < added_faces.size(); ++i)
		{
			std::vector<CPoint> points = db_landmark[added_faces[i]];
			ss.str("");
			ss << existed_image_id;
			string sql_insert_faces = "insert into Faces values(NULL," + ss.str();
			ss.str("");

			for (int i = 0; i < points.size(); ++i)
			{
				ss << points[i].x;
				sql_insert_faces += "," + ss.str();
				ss.str("");
				ss << points[i].y;
				sql_insert_faces += "," + ss.str();
				ss.str("");
			}
			for (int i = 0; i < textFieldName.size(); ++i)
			{
				sql_insert_faces += ",'" + db_pro_values[added_faces[i]][textFieldName[i]] + "'";
			}
			sql_insert_faces += ")";
			const char *sql = sql_insert_faces.c_str();
			rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
		}

		std::map<int, std::vector<CPoint>>::iterator it;
		for (it = db_landmark.begin(); it != db_landmark.end(); ++it)
		{
			if (find(added_faces.begin(), added_faces.end(), it->first) == added_faces.end()
				&& find(erased_faces.begin(), erased_faces.end(), it->first) == erased_faces.end())
			{
				std::vector<CPoint> points = it->second;
				string sql_update = "update Faces set ";
				ss.str("");
				ss << points[0].x;
				sql_update += keyPoint_fieldName[0] + "=" + ss.str();
				ss.str("");
				ss << points[0].y;
				sql_update += "," + keyPoint_fieldName[1] + "=" + ss.str();
				ss.str("");

				for (int i = 1; i < points.size(); ++i)
				{
					ss << points[i].x;
					sql_update += "," + keyPoint_fieldName[i * 2] + "=" + ss.str();
					ss.str("");
					ss << points[i].y;
					sql_update += "," + keyPoint_fieldName[i * 2 + 1] + "=" + ss.str();
					ss.str("");
				}
				for (int i = 0; i < textFieldName.size(); ++i)
				{
					sql_update += ", " + textFieldName[i] + "='" + db_pro_values[it->first][textFieldName[i]] + "'";
				}
				ss << it->first;
				sql_update += " where face_id = " + ss.str();
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

	if (sock)
	{
		char buff[7] = "op_end";
		send(sockClient, buff, sizeof(buff), 0);
		closesocket(sockClient);
	}
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
	std::vector<CString>::iterator it = find(Images.begin(), Images.end(), imageName);
	if (it!=Images.end())
	{
		Images.erase(it);
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

std::string CLandmarkEditDlg::encode(const std::string to_encode)
{
	std::string res = to_encode;
	std::string key = "4962873";
	for (int i = 0, j = 0; res[j]; j++, i = (i + 1) % 7){
		res[j] += key[i];
		if (res[j] > 122) res[j] -= 90;
	}
	return res;
}

std::string CLandmarkEditDlg::decode(const std::string to_decode)
{
	std::string key = "4962873";
	std::string res = to_decode;
	for (int i = 0, j = 0; res[j]; j++, i = (i + 1) % 7){
		res[j] -= key[i];
		if (res[j] < 32) res[j] += 90;
	}
	return res;
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

void CLandmarkEditDlg::ConnectServer()
{
	//加载套接字
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		TRACE("Failed to load Winsock");
		return;
	}

	string txtPath = save_path + "\\ip.txt";
	ifstream infile(txtPath);
	string ip;
	getline(infile, ip);
	infile.close();

	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(2501);
	addrSrv.sin_addr.S_un.S_addr = inet_addr(ip.c_str());

	//创建套接字
	sockClient = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == sockClient){
		TRACE("Socket() error:%d", WSAGetLastError());
		return;
	}

	//向服务器发出连接请求
	if (connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(addrSrv)) == INVALID_SOCKET){
		TRACE("Connect failed:%d", WSAGetLastError());
		return;
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

void CLandmarkEditDlg::GetProNameAndValues(std::vector<string> & pro_name, std::vector<string> &profileFieldName,
	std::map<string, std::vector<string>> & landmark_pro_values,
	std::map<string, std::vector<string>> & profile_pro_values)
{

	string cfgPath = save_path + "\\pro_values.ini";
	CFileFind fileFinder;
	if (fileFinder.FindFile(CString(cfgPath.c_str())))
	{
		ifstream infile(cfgPath);
		string lines;
		while (getline(infile, lines))
		{
			std::vector<string> info = split(lines, " ");
			int type = stoi(info[0]);
			string field_name = split(info[1], ":")[0];
			string field_values = split(info[1], ":")[1];
			std::vector<string> values = split(field_values, ",");
			if (type == 0)
			{
				pro_name.push_back(field_name);
				landmark_pro_values[field_name] = values;
			}
			else
			{
				profileFieldName.push_back(field_name);
				profile_pro_values[field_name] = values;
			}

		}
		infile.close();
	}
}