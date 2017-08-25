
// MFCApplication2Dlg.cpp : implementation file
#include "stdafx.h"
#include "MFCApplication2.h"
#include "MFCApplication2Dlg.h"
#include "afxdialogex.h"
#include "verify.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define N 129
#define LIST_WIDTH 1350
#define LIST_HEIGHT 750
#define MAX_COUNT 30

int LEFT_BEGIN = 40;
int TOP_BEGIN = 80;
int IMG_WIDTH = 150;
int IMG_HEIGHT = 150;

CRect MainRect; //客户区
CString basePath = L"";  //当前处理图片所在路径
CString parentPath = L"";//当前处理图片路径的上级路径
bool MergeOptionDone = FALSE; //判断是否进行了合并操作
float threshold = 0.4;
CDC *pDC = NULL;
void* m_pMutex;

// CAboutDlg dialog used for App About
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
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
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
	SetTimer(1,500, NULL);
	return TRUE;
}

void CMessageDlg::OnTimer(UINT_PTR nIDEvent)
{
	CDialogEx::OnTimer(nIDEvent);
	EndDialog(0);
}

// CMFCApplication2Dlg dialog
CMFCApplication2Dlg::CMFCApplication2Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMFCApplication2Dlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCApplication2Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_listCtrl);
}

BEGIN_MESSAGE_MAP(CMFCApplication2Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_CLOSE()
	ON_NOTIFY(NM_CLICK, IDC_LIST1, &CMFCApplication2Dlg::OnNMClickList1)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, &CMFCApplication2Dlg::OnNMDbClickList1)
	ON_BN_CLICKED(IDC_MERGE, &CMFCApplication2Dlg::OnBnClickedMerge)
	ON_BN_CLICKED(IDC_NEXT_PIC, &CMFCApplication2Dlg::OnBnClickedNextPic)
	ON_COMMAND(ID_GET_FEATURE, &CMFCApplication2Dlg::OnGetFeature)
	ON_BN_CLICKED(IDC_GETEDIT, &CMFCApplication2Dlg::OnBnClickedGetedit)
END_MESSAGE_MAP()

// CMFCApplication2Dlg message handlers
BOOL CMFCApplication2Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	///CWnd::SetWindowPos(NULL, 0, 0, GetSystemMetrics(SM_CXFULLSCREEN), GetSystemMetrics(SM_CYFULLSCREEN), SWP_NOZORDER | SWP_NOMOVE);
	ShowWindow(SW_SHOWMAXIMIZED);

	SetWindowText(L"图片筛选工具");
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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_imageList.Create(200, 200, ILC_COLORDDB | ILC_MASK, 4, 2);
	m_listCtrl.SetImageList(&m_imageList, LVSIL_NORMAL);

	GetClientRect(&MainRect);
	SetScrollRange(SB_VERT, 0, MainRect.bottom - MainRect.top, TRUE);

	CEdit *edit = (CEdit*)GetDlgItem(IDC_EDIT1);
	edit->SetWindowTextW(L"0.4");
	
	pDC = GetDC();

	CRect list_rect;
	GetDlgItem(IDC_LIST1)->GetWindowRect(&list_rect);
	ScreenToClient(&list_rect);
	GetDlgItem(IDC_LIST1)->MoveWindow(300, TOP_BEGIN, LIST_WIDTH, LIST_HEIGHT, TRUE);

	//DWORD listStyle = m_listCtrl.GetExtendedStyle();
	//listStyle |= LVS_AUTOARRANGE;
	//m_listCtrl.SetExtendedStyle(listStyle);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMFCApplication2Dlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMFCApplication2Dlg::OnPaint()
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

		CFileFind finder;
		if (finder.FindFile(current))
		{
			ShowImage(LEFT_BEGIN, TOP_BEGIN, current);
			ShowDetailedImage();
		}
	}
}

HCURSOR CMFCApplication2Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMFCApplication2Dlg::addImage(CString path, CString image, float score)
{
	CBitmap bitmap;
	CImage img;
	CFileFind finder;
	if (!finder.FindFile(path))
	{
		return;
	}
	HRESULT ret = img.Load(path); 
	HBITMAP hbitmap = img.Detach();

	HANDLE hBB = CopyImage((HANDLE)hbitmap, IMAGE_BITMAP, 200, 200, LR_COPYRETURNORG);
	hbitmap = (HBITMAP)hBB;

	bitmap.Attach(hbitmap);

	int n = m_imageList.Add(&bitmap, RGB(255, 0, 255));

	CString strItem;
	CString strItem2;
	strItem2.Format(_T(":%f"),score);
	strItem = image + strItem2;

	LVITEM lvi;
	lvi.mask = LVIF_IMAGE | LVIF_TEXT;
	lvi.iItem = n ;
	lvi.iSubItem = 0;
	lvi.pszText = (LPTSTR)(LPCTSTR)(strItem);
	lvi.iImage = n;
	m_listCtrl.InsertItem(&lvi);

	img.Destroy();
}

void CMFCApplication2Dlg::ShowImage(int l, int t, CString imagePath)
{
	CFileFind fileFinder;
	if (fileFinder.FindFile(imagePath))
	{
		CImage  image;
		image.Load(imagePath);

		pDC->SetStretchBltMode(HALFTONE);
		CRect rect(l, t, IMG_WIDTH + l, IMG_HEIGHT + t);
		image.Draw(pDC->m_hDC, rect);

		image.Destroy();
	}
}

void CMFCApplication2Dlg::ChangeSize(UINT nID, int x, int y)
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

void CMFCApplication2Dlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if (nType != SIZE_MINIMIZED)
	{
		ChangeSize(IDC_STATIC_TXT, cx, cy);
		ChangeSize(IDC_EDIT1, cx, cy);
		ChangeSize(IDC_GETEDIT, cx, cy);
		ChangeSize(IDC_MERGE, cx, cy);
		ChangeSize(IDC_NEXT_PIC, cx, cy);
		GetClientRect(&MainRect);
	}
}

void CMFCApplication2Dlg::OnNMClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
}

void CMFCApplication2Dlg::OnNMDbClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (!dbclicked.empty())
	{
		dbclicked.clear();
	}
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	int nIndex = pNMListView->iItem;

	CString item_text = m_listCtrl.GetItemText(nIndex, 0);
	if (!item_text.IsEmpty())
	{
		string tmp = CStringA(item_text);
		dbclicked = split(tmp, ".")[0];

		ShowDBCilckedImage();
	}
}

void CMFCApplication2Dlg::OnBnClickedMerge()
{
	MergeSelected();
}

void CMFCApplication2Dlg::OnBnClickedNextPic()
{
	if (MergeOptionDone == FALSE)
	{
		CFileFind fileFinder;
		CString temp_from = basePath + currentImageName;
		if (!fileFinder.FindFile(temp_from))
		{
			AfxMessageBox(L"该图片已经被合并!!!", MB_OK | MB_ICONINFORMATION);

			GetNextImage();
		}
		else
		{
			if (matched.empty())
			{
				CString cate = basePath + currentImageName.SpanExcluding(L".jpg");
				CreateDirectory(cate, 0);
				CString temp_to = cate + "\\" + currentImageName;
				MoveFileEx(temp_from, temp_to, MOVEFILE_REPLACE_EXISTING);

				GetNextImage();
			}
			else
			{
				GetSelectedImage();
				if (selected.empty())
				{
					if (AfxMessageBox(L"您未选中任何图片，是否获取下一张图片？", MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
					{
						CString cate = basePath + currentImageName.SpanExcluding(L".jpg");
						CreateDirectory(cate, 0);
						CString temp_to = cate + "\\" + currentImageName;
						MoveFileEx(temp_from, temp_to, MOVEFILE_REPLACE_EXISTING);
					}

					GetNextImage();
				}
				else
				{
					selected.clear();
					AfxMessageBox(L"请先进行合并操作!", MB_OK | MB_ICONERROR);
				}
			}
		}	
	}
	else
	{
		MergeOptionDone = FALSE;
		GetNextImage();
	}
}

void CMFCApplication2Dlg::OnGetFeature()
{
	CFileDialog selectPic(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, L"TXT(*.txt)|*.txt||");
	if (IDOK == selectPic.DoModal())
	{
		if (!landmark.empty())
		{
			landmark.clear();
		}
		string txtPath = CStringA(selectPic.GetPathName());
		CString folder_path = selectPic.GetFolderPath();
		basePath = folder_path + L"\\";
		
		string f_path = (CStringA)folder_path;
		string p_path = f_path.substr(0, f_path.rfind("\\", f_path.size()));

		parentPath = CString(p_path.c_str()) + L"\\";

		readLandmark(txtPath);
		GetRemainingLandmark();

		/*ofstream test("log.txt");
		map<string, vector<float>>::iterator it;
		for (it = landmark.begin(); it != landmark.end();++it)
		{
			CString img_tmp = basePath + CString((it->first).c_str());
			CFileFind finder;
			if (!finder.FindFile(img_tmp))
			{
				test << it->first << endl;
			}
		}*/
		if (!landmark.empty()){
			string s = GetUniqueImage();
			currentImageName = CString(s.c_str());
			current = basePath + currentImageName;

			GetDlgItem(IDC_CT_NAME)->SetWindowTextW(currentImageName);

			if (!selected.empty())
			{
				selected.clear();
			}
			if (!matched.empty())
			{
				matched.erase(matched.begin(), matched.end());
			}
			m_imageList.DeleteImageList();
			m_listCtrl.DeleteAllItems();

			m_imageList.Create(200, 200, ILC_COLORDDB | ILC_MASK, 4, 2);
			m_listCtrl.SetImageList(&m_imageList, LVSIL_NORMAL);

			CFileFind finder;
			if (finder.FindFile(current))
			{
				ShowImage(LEFT_BEGIN, TOP_BEGIN, current);
				ShowDetailedImage();

				GetMatchedImage();
				ShowMatchedImage();
			}
		}
	}
}

void CMFCApplication2Dlg::OnBnClickedGetedit()
{
	CEdit *edit = (CEdit*)GetDlgItem(IDC_EDIT1);
	CString edit_text;
	edit->GetWindowTextW(edit_text);
	threshold = _tstof(edit_text);

	if (!matched.empty())
	{
		matched.erase(matched.begin(), matched.end());
	}

	m_imageList.DeleteImageList();
	m_listCtrl.DeleteAllItems();

	m_imageList.Create(200, 200, ILC_COLORDDB | ILC_MASK, 4, 2);
	m_listCtrl.SetImageList(&m_imageList, LVSIL_NORMAL);

	GetMatchedImage();
	ShowMatchedImage();
}

void CMFCApplication2Dlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SCROLLINFO scrollInfo;
	GetScrollInfo(SB_VERT, &scrollInfo, SIF_ALL);
	switch (nSBCode)
	{
	case SB_LINEUP:
		scrollInfo.nPos -= 1;
		if (scrollInfo.nPos < scrollInfo.nMin)
		{
			scrollInfo.nPos = scrollInfo.nMin;
			break;
		}
		SetScrollInfo(SB_VERT, &scrollInfo, SIF_ALL);
		ScrollWindow(0, 1);
		break;
	case SB_LINEDOWN:
		scrollInfo.nPos += 1;
		if (scrollInfo.nPos > scrollInfo.nMax)
		{
			scrollInfo.nPos = scrollInfo.nMax;
			break;
		}
		SetScrollInfo(SB_VERT, &scrollInfo, SIF_ALL);
		ScrollWindow(0, -1);
		break;
	case SB_TOP:
		ScrollWindow(0, (scrollInfo.nPos - scrollInfo.nMin) * 1);
		scrollInfo.nPos = scrollInfo.nMin;
		SetScrollInfo(SB_VERT, &scrollInfo, SIF_ALL);
		break;
	case   SB_BOTTOM:
		ScrollWindow(0, -(scrollInfo.nMax - scrollInfo.nPos) * 1);
		scrollInfo.nPos = scrollInfo.nMax;
		SetScrollInfo(SB_VERT, &scrollInfo, SIF_ALL);
		break;
	case   SB_PAGEUP:
		scrollInfo.nPos -= 1;
		if (scrollInfo.nPos < scrollInfo.nMin)
		{
			scrollInfo.nPos = scrollInfo.nMin;
			break;
		}
		SetScrollInfo(SB_VERT, &scrollInfo, SIF_ALL);
		ScrollWindow(0, 1 * 1);
		break;
	case SB_PAGEDOWN:
		scrollInfo.nPos += 1;
		if (scrollInfo.nPos > scrollInfo.nMax)
		{
			scrollInfo.nPos = scrollInfo.nMax;
			break;
		}
		SetScrollInfo(SB_VERT, &scrollInfo, SIF_ALL);
		ScrollWindow(0, -1 * 1);
		break;
	case   SB_ENDSCROLL:
		break;
	case SB_THUMBPOSITION:
		break;
	case SB_THUMBTRACK:
		ScrollWindow(0, (scrollInfo.nPos - nPos));
		scrollInfo.nPos = nPos;
		SetScrollInfo(SB_VERT, &scrollInfo, SIF_ALL);
		break;
	}
}

void CMFCApplication2Dlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SCROLLINFO scrollInfo;
	GetScrollInfo(SB_HORZ, &scrollInfo, SIF_ALL);
	switch (nSBCode)
	{
	case SB_LINEUP:
		scrollInfo.nPos -= 1;
		if (scrollInfo.nPos < scrollInfo.nMin)
		{
			scrollInfo.nPos = scrollInfo.nMin;
			break;
		}
		SetScrollInfo(SB_HORZ, &scrollInfo, SIF_ALL);
		ScrollWindow(10, 0);
		break;
	case SB_LINEDOWN:
		scrollInfo.nPos += 1;
		if (scrollInfo.nPos > scrollInfo.nMax)
		{
			scrollInfo.nPos = scrollInfo.nMax;
			break;
		}
		SetScrollInfo(SB_HORZ, &scrollInfo, SIF_ALL);
		ScrollWindow(-10, 0);
		break;
	case SB_TOP:
		ScrollWindow((scrollInfo.nPos - scrollInfo.nMin) * 10, 0);
		scrollInfo.nPos = scrollInfo.nMin;
		SetScrollInfo(SB_HORZ, &scrollInfo, SIF_ALL);
		break;
	case SB_BOTTOM:
		ScrollWindow(-(scrollInfo.nMax - scrollInfo.nPos) * 10, 0);
		scrollInfo.nPos = scrollInfo.nMax;
		SetScrollInfo(SB_HORZ, &scrollInfo, SIF_ALL);
		break;
	case SB_PAGEUP:
		scrollInfo.nPos -= 5;
		if (scrollInfo.nPos < scrollInfo.nMin)
		{
			scrollInfo.nPos = scrollInfo.nMin;
			break;
		}
		SetScrollInfo(SB_HORZ, &scrollInfo, SIF_ALL);
		ScrollWindow(10 * 5, 0);
		break;
	case SB_PAGEDOWN:
		scrollInfo.nPos += 5;
		if (scrollInfo.nPos > scrollInfo.nMax)
		{
			scrollInfo.nPos = scrollInfo.nMax;
			break;
		}
		SetScrollInfo(SB_HORZ, &scrollInfo, SIF_ALL);
		ScrollWindow(-10 * 5, 0);
		break;
	case SB_ENDSCROLL:
		break;
	case SB_THUMBPOSITION:
		break;
	case SB_THUMBTRACK:
		ScrollWindow((scrollInfo.nPos - nPos) * 10, 0);
		scrollInfo.nPos = nPos;
		SetScrollInfo(SB_HORZ, &scrollInfo, SIF_ALL);
		break;
	}
}

void CMFCApplication2Dlg::OnClose()
{
	if (!MergeOptionDone)
	{
		if (AfxMessageBox(L"请确保当前图片已经被合并！！！", MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
		{
			CDialogEx::OnClose();
		}
	}
	else
	{
		CDialogEx::OnClose();
	}
}

BOOL CMFCApplication2Dlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
		return TRUE;
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
		return TRUE;
	else
		return CDialog::PreTranslateMessage(pMsg);
}

vector<string> CMFCApplication2Dlg::split(string str, string separator)
{
	vector<string> result;
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

void  CMFCApplication2Dlg::readLandmark(string txtPath)
{
	ifstream fopen(txtPath);
	string s;
	vector<string> temp;
	while (getline(fopen, s))
	{
		temp = split(s, ",");
		vector<float> vf(128, 0);
		for (int i = 0; i < 128; i++){
			vf[i] = atof(temp[i + 2].c_str());
		}
		feature[split(temp[1], "\\")[1]] = vf;
	}
	fopen.close();
}

void CMFCApplication2Dlg::GetMatchedImage()
{
	vector<float> currentImage;
	string current_s = CStringA(currentImageName);
	map<string, vector<float>>::iterator it = landmark.find(current_s);
	if (it != landmark.end())
	{
		currentImage = it->second;
	}
	for (it = landmark.begin(); it != landmark.end(); ++it)
	{
		if (it->first != current_s)
		{
			vector<float> tmp = it->second;
			float score = verify(&currentImage[0], &tmp[0], N);
			if (score >= threshold)
			{
				matched[score] = it->first;
			}
			tmp.clear();
		}
	}
	currentImage.clear();
}

void CMFCApplication2Dlg::ShowMatchedImage()
{
	map<float, string, greater<float> >::iterator it;
	int count = 0;
	for (it = matched.begin(); it != matched.end(); ++it)
	{
		string tmp = it->second;
		CString image = CString(tmp.c_str());
		CString p = basePath + image;
		addImage(p, image, it->first);
		if (++count >= 30)
		{
			break;
		}
	}
}

void CMFCApplication2Dlg::GetNextImage()
{
	if (!remaining.empty())
	{
		remaining.clear();
	}
	if (!landmark.empty())
	{
		landmark.clear();
	}
	GetRemainingLandmark();

	if (!landmark.empty())
	{
		string s = GetUniqueImage();
		currentImageName = CString(s.c_str());
		current = basePath + currentImageName;

		GetDlgItem(IDC_CT_NAME)->SetWindowTextW(currentImageName);

		if (!selected.empty())
		{
			selected.clear();
		}
		if (!matched.empty())
		{
			matched.erase(matched.begin(), matched.end());
		}
		if (!dbclicked.empty())
		{
			dbclicked.clear();
		}
		m_imageList.DeleteImageList();
		m_listCtrl.DeleteAllItems();

		m_imageList.Create(200, 200, ILC_COLORDDB | ILC_MASK, 4, 2);
		m_listCtrl.SetImageList(&m_imageList, LVSIL_NORMAL);

		CFileFind finder;
		if (finder.FindFile(current))
		{
			ShowImage(LEFT_BEGIN, TOP_BEGIN, current);
			ShowDetailedImage();

			GetMatchedImage();
			ShowMatchedImage();
		}
		else
		{
			MergeOptionDone = true;
		}
	}
	else
	{
		if (AfxMessageBox(L"没有需要处理的图片！",MB_OK|MB_ICONINFORMATION) == IDOK)
		{
			m_imageList.DeleteImageList();
			m_listCtrl.DeleteAllItems();
		}
	}
}

void CMFCApplication2Dlg::MergeSelected()
{
	if (feature.empty())
	{
		AfxMessageBox(L"请先获取特征数据文件!", MB_OK | MB_ICONERROR);
	}
	else
	{
		if (landmark.empty())
		{
			AfxMessageBox(L"所有图片处理完成!", MB_OK | MB_ICONINFORMATION);
		}
		else
		{
			if (!MergeOptionDone)
			{
				if (MergeLock())
				{
					CFileFind fileFinder;
					CString tp = basePath + currentImageName;
					if (!fileFinder.FindFile(tp))
					{
						AfxMessageBox(L"该图片已经被合并!!!", MB_OK | MB_ICONINFORMATION);
						if (!selected.empty())
						{
							selected.clear();
						}
						GetNextImage();
					}
					else
					{
						GetSelectedImage();
						CString selectedNum;
						selectedNum.Format(L"%d", selected.size());
						CString showMessage = selectedNum + L" 张图片被选中，是否确认合并？";
						if ((AfxMessageBox(showMessage, MB_OKCANCEL | MB_ICONINFORMATION) == IDOK))
						{
							CString cate = basePath + currentImageName.SpanExcluding(L".jpg");
							CreateDirectory(cate, 0);
							CString temp_to = cate + "\\" + currentImageName;
							CString temp_from = basePath + currentImageName;
							MoveFileEx(temp_from, temp_to, MOVEFILE_REPLACE_EXISTING);

							for (int i = 0; i < selected.size(); ++i)
							{
								string erased = selected[i];
								map<string, vector<float>>::iterator it = landmark.find(erased);
								if (it != landmark.end())
								{
									landmark.erase(erased);
								}

								CString to = cate + "\\" + selected[i].c_str();
								CString from = basePath + selected[i].c_str();
								CFileFind fileFinder;
								if (fileFinder.FindFile(from))
								{
									MoveFileEx(from, to, MOVEFILE_REPLACE_EXISTING);
								}
							}
							selected.clear();
							MergeOptionDone = TRUE;
						}
						else
						{
							selected.clear();
							MergeUnlock();
						}
					}
					MergeUnlock();
				}
				else
				{
					AfxMessageBox(L"当前无法进行合并操作，请等待！", MB_OK | MB_ICONINFORMATION);
				}
			}
			else
			{
				AfxMessageBox(L"合并操作已完成，请勿重复操作！", MB_OK | MB_ICONERROR);
			}
		}
		
	}
}

void CMFCApplication2Dlg::GetRemainingImages()
{
	CFileFind finder;
	BOOL working = finder.FindFile(basePath + "*.jpg");
	while (working)
	{
		working = finder.FindNextFile();
		if (finder.IsDots())
			continue;
		if (!finder.IsDirectory())
		{
			string filename = CStringA(finder.GetFileName());
			remaining.push_back(filename);
		}
	}
	finder.Close();
}

void CMFCApplication2Dlg::GetSelectedImage()
{
	POSITION pos = m_listCtrl.GetFirstSelectedItemPosition();
	while (pos){
		int nIndex = m_listCtrl.GetNextSelectedItem(pos);
		CString item_text = m_listCtrl.GetItemText(nIndex, 0);
		if (!item_text.IsEmpty())
		{
			string tmp = CStringA(item_text);
			string picked = split(tmp, ":")[0];
			if (find(selected.begin(), selected.end(), picked) == selected.end())
			{
				selected.push_back(picked);
			}
		}
	}
}

void CMFCApplication2Dlg::ShowDetailedImage()
{
	CString cate = parentPath + currentImageName.SpanExcluding(L".jpg") + L"\\";
	vector<CString> detailed;

	CFileFind finder;
	BOOL working = finder.FindFile(cate + "*.jpg");
	int count = 1;
	while (working && count++ <= 3)
	{
		working = finder.FindNextFile();
		if (finder.IsDots())
			continue;
		if (!finder.IsDirectory())
		{
			detailed.push_back(finder.GetFilePath());
		}
	}
	finder.Close();
	for (int i = 0; i < detailed.size(); ++i)
	{
		ShowImage(LEFT_BEGIN, TOP_BEGIN + IMG_HEIGHT * (i + 1), detailed[i]);
	}
}

void CMFCApplication2Dlg::ShowDBCilckedImage()
{
	if (!dbclicked.empty())
	{
		CString cate = parentPath + dbclicked.c_str() + L"\\";

		vector<CString> detailed;

		CFileFind finder;
		BOOL working = finder.FindFile(cate + "*.jpg");
		int count = 1;
		while (working && count++ <= 3)
		{
			working = finder.FindNextFile();
			if (finder.IsDots())
				continue;
			if (!finder.IsDirectory())
			{
				detailed.push_back(finder.GetFilePath());
			}
		}
		finder.Close();

		for (int i = 0; i < detailed.size(); ++i)
		{
			ShowImage(300 + i * IMG_WIDTH, LIST_HEIGHT + TOP_BEGIN, detailed[i]);
		}
		if (!detailed.empty())
		{
			detailed.clear();
		}
	}
}

void CMFCApplication2Dlg::GetRemainingLandmark()
{
	GetRemainingImages();
	vector<string>::iterator remaining_it;
	for (remaining_it = remaining.begin(); remaining_it != remaining.end(); ++remaining_it)
	{
		landmark[*remaining_it] = feature[*remaining_it];
	}
}

bool CMFCApplication2Dlg::MergeLock()
{
	bool res = false;
	if (Lock())
	{
		string txtPath = CStringA(basePath) + "merge_lock.txt";
		ifstream infile(txtPath);
		int i;
		infile >> i;
		infile.close();

		if (i == 0)
		{
			res = true;
			ofstream outfile(txtPath, ios::trunc);
			outfile << 1;
			outfile.close();
		}

		UnLock();
	}
	return res;
}

void CMFCApplication2Dlg::MergeUnlock()
{
	if (Lock())
	{
		string txtPath = CStringA(basePath) + "merge_lock.txt";
		ofstream outfile(txtPath, ios::trunc);
		outfile << 0;
		outfile.close();

		UnLock();
	}
}

string CMFCApplication2Dlg::GetUniqueImage()
{
	CMessageDlg message_dlg;
	message_dlg.DoModal();

	string res = "";
	if (Lock())
	{
		string txtPath = basePath + "image.txt";
		ifstream infile(txtPath);
		string s;
		vector<string> temp;
		while (getline(infile, s))
		{
			temp.push_back(s);
		}
		infile.close();

		map<string, vector<float>>::iterator it;
		for (it = landmark.begin(); it != landmark.end(); ++it)
		{
			if (find(temp.begin(), temp.end(), it->first) == temp.end())
			{
				CFileFind finder;
				string tmp = it->first;
				CString image = basePath + CString(tmp.c_str());
				if (finder.FindFile(image))
				{
					res = it->first;
					break;
				}
			}
		}

		ofstream outfile(txtPath, ios::app);
		outfile << res <<endl;
		outfile.close();

		UnLock();
	}
	return res;
}

bool CMFCApplication2Dlg::Lock()
{
	m_pMutex = CreateMutex(NULL, false, L"txt_mutex");
	if (NULL == m_pMutex)
	{
		return false;
	}
	DWORD nRet = WaitForSingleObject(m_pMutex, INFINITE);
	if (nRet != WAIT_OBJECT_0)
	{
		return false;
	}
	return true;
}

bool CMFCApplication2Dlg::UnLock()
{
	return ReleaseMutex(&m_pMutex);
}
