
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
	CString current; //��ǰ����ͼ��·��
	CString currentImageName;//
	void addImage(CString path, CString image, float score);//��ͼƬ��ӵ�imagelist
	void ShowImage(int l, int t,CString imagePath);  //��ʾ��ǰ�����ͼƬ
	void ChangeSize(UINT nID, int x, int y);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNMClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDbClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedMerge();
	afx_msg void OnBnClickedNextPic();
	afx_msg void OnGetFeature();  //��ȡ����ͼƬ������Ϣ��txt�ļ�
	afx_msg void OnBnClickedGetedit();//��ȡ�༭�������µ���ֵ
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnClose();
public:
	map<float, string, greater<float> > matched;  //ƥ���������0.4��ͼ�������Ϣ
	map<string, vector<float>> landmark;  //��ǰ��Ҫ�����landmark��Ϣ
	map<string, vector<float>> feature;  //��ȡtxt�ļ��е�����landmark��Ϣ
	vector<string> selected;  //ѡ�е�ͼƬ��
	vector<string> remaining;  //��ǰ�����������ͼƬ��
	string dbclicked;
	vector<string> split(string str, string separator);//�ַ�������
	void readLandmark(string txtPath); //��txt�ļ��ж�ȡ����ͼƬ������Ϣ��������feature
	void GetMatchedImage();  //��ȡ����ǰͼ��ƥ��ֵ����0.4������ͼƬ���ƺ�ƥ����� ������matched���ݽṹ
	void ShowMatchedImage();  //��matched�е�ͼƬ��ʾ��Clistctrl�ؼ�
	void GetNextImage();  //��ȡ��һ�Ŵ�����ͼƬ
	void MergeSelected();  //��ѡ�е�ͼƬ�ϲ���һ���µ��ļ���
	void GetRemainingImages();  //��ȡ�ļ���������δ����ͼƬ������Ϣ ������landmark
	void GetSelectedImage();  //��ȡѡ��ͼƬ��Ϣ ������selected��
	void ShowDetailedImage();//��ȡ3���뵱ǰͼ������һ����ͼ��
	void ShowDBCilckedImage();//˫����ʾ3��ͼƬ
	void GetRemainingLandmark();
	bool MergeLock();
	void MergeUnlock();
	string GetUniqueImage();
	bool Lock();
	bool UnLock();
};
