
// MFCApplication2Dlg.h : header file
#pragma once
#include<vector>
#include<fstream>
#include<map>
#include<string>
#include<math.h>
#include<functional>
#include<iostream>
#include<time.h>
#include<algorithm>
using namespace std;


// CMFCApplication2Dlg dialog
class CMFCApplication2Dlg : public CDialogEx
{
// Construction
public:
	CMFCApplication2Dlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_MFCAPPLICATION2_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_listCtrl;
	CImageList m_imageList;
	CString current; //当前处理图像路径
	CString currentImageName;//
	void addImage(CString path, CString image, float score);//将图片添加到imagelist
	void ShowImage(int l, int t,CString imagePath);  //显示当前处理的图片
	void ChangeSize(UINT nID, int x, int y);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNMClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDbClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedMerge();
	afx_msg void OnBnClickedNextPic();
	afx_msg void OnGetFeature();  //获取含有图片特征信息的txt文件
	afx_msg void OnBnClickedGetedit();//获取编辑框中最新的阈值
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnClose();
public:
	map<float, string, greater<float> > matched;  //匹配分数大于0.4的图像相关信息
	map<string, vector<float>> landmark;  //当前需要处理的landmark信息
	map<string, vector<float>> feature;  //读取txt文件中的所有landmark信息
	vector<string> selected;  //选中的图片名
	vector<string> remaining;  //当前待处理的所有图片名
	string dbclicked;
	vector<string> split(string str, string separator);//字符串划分
	void readLandmark(string txtPath); //从txt文件中读取所有图片特征信息，保存至feature
	void GetMatchedImage();  //获取跟当前图像匹配值大于0.4的所有图片名称和匹配分数 保存至matched数据结构
	void ShowMatchedImage();  //将matched中的图片显示到Clistctrl控件
	void GetNextImage();  //获取下一张待处理图片
	void MergeSelected();  //将选中的图片合并到一个新的文件夹
	void GetRemainingImages();  //获取文件夹中所有未处理图片特征信息 保存至landmark
	void GetSelectedImage();  //获取选中图片信息 保存至selected中
	void ShowDetailedImage();//获取3张与当前图像人物一样的图像
	void ShowDBCilckedImage();//双击显示3张图片
	void GetRemainingLandmark();
	bool MergeLock();
	void MergeUnlock();
	string GetUniqueImage();
	bool Lock();
	bool UnLock();
};
