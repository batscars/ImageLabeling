
// LandmarkEditDlg.h : header file
//

#pragma once
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <functional>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>  
#include <opencv2/highgui/highgui.hpp>  
#include <math.h>
#include <time.h>
extern "C"
{
  #include "sqlite3.h"
};
using namespace std;

#pragma comment (lib, "sqlite3.lib")

// CLandmarkEditDlg dialog
class CLandmarkEditDlg : public CDialogEx
{
// Construction
public:
	CLandmarkEditDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_LANDMARKEDIT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	
	//�ؼ�������Ϣ������
public:
	void ChangeSize(UINT nID, int x, int y);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnOpenPic();
	afx_msg void OnEditPro();
    afx_msg void OnDeleteFace();
	afx_msg void OnBnClickedNextPic();
	afx_msg void OnBnClickedSearch();
	afx_msg void OnNMClickList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedSave();
	afx_msg void OnAddOperator();
	afx_msg void OnExit();
	afx_msg void OnClose();
	afx_msg void OnChangePwd();
	afx_msg void OnAddPro();
	afx_msg void OnBnClickedDelPic();
	afx_msg void OnDelOperator();
	afx_msg void OnBodyRect();
	afx_msg void OnUpperRect();
	afx_msg void OnDeleteFigure();
	afx_msg void OnMovRect();
	afx_msg void OnCancelRect();

	//��Ա����
public:
	std::map<int, CRect> db_rect;                                            //figure_id --- ȫ����ο�
	std::map<int, CRect> db_upper_rect;                                      //figure_id --- �ϰ�����ο�
	std::map<int, CRect> db_lower_rect;                                      //figure_id --- �°�����ο�
	std::vector<CString> Images;                                             //�ļ�����ʣ�������ͼƬ
	map<int, map<string, string, greater<string>>> db_pro_values;
	CListCtrl m_listCtrl;                                                    //��ѯ����б�
	CDateTimeCtrl m_dateStart;                                               //��ѯ��ʼ����
	CDateTimeCtrl m_dateEnd;                                                 //��ѯ��ֹ����

	//ȫ�֡���Ա�������¼���ͼ����
public:
	void ShowImage(CString imagePath);                                       //��ʾ��ǰ�����ͼƬ                             
	void DrawOnBuffer();                                                     //˫�����ͼ
	void SaveOption();                                                       //�������
	void DrawRectangle(CRect rect, COLORREF color);                          //���ڴ���ƾ���
	void DrawRectangle(CPoint X, CPoint Y);                                  //���ڴ���ƾ���
	void DrawEclipse(int x, int y, int r, COLORREF color);
	void DrawLine(int nPenStyle, CPoint x, CPoint y, COLORREF color);
	void DrawResizePoints();                                                 //���ڴ���ƿ�Ų���ĵ�

	std::map<string, COLORREF> InitColorRGB();                               //��ʼ����ɫ��Ӧ��rgbֵ--���ڻ�����ɫ������
	std::map<string, cv::Scalar> InitColorScalar();                          //��ʼ����ɫ��Ӧ��scalarֵ--���ڱȽ���ɫ���ƶ�
	void InitProValues();                        //��ȡ�������Կ�ѡ������ֵ
	std::vector<string> GetFigureFieldName(string res);                      //��ȡfigures�������пɱ༭����
	int GetCurFigureId(CPoint x);                                            //��������һ�λ�û�ȡ����id
	int GetCurrentFigure(std::map<int, CRect> rect, CPoint x, CPoint y);  //���ݾ��ο��λ�û�ȡ��ǰ����id
	std::vector<CString> GetRemainingImages(CString basePath);               //��ȡ��ǰ����·���е�����ͼƬ���Ʊ�����vector��
	void InitRects(int image_id);                                            //����image_id��ȡ�������������ο�

	//���ݿ���ز���
public:
	/*
	�û�������غ���
	*/
	bool DB_AllowLogin(CString operator_id, CString password);               //�Ƿ��¼�ɹ�
	bool DB_OperatorExisted();                                               //operator�����Ƿ���ڼ�¼
	bool DB_IsAdmin(CString operator_id);                                    //�жϵ�ǰ�û��Ƿ�Ϊ����Ա
	CString DB_GetPwd(CString operator_id);                                  //��ȡ��ǰ�����û�id������
	std::map<string, string> DB_GetOperators();                              //��ȡ������Ա��������û�id������
	bool DB_InsertOperator(CString operator_id, CString op_name,
		CString password, CString permission, CString state);                //����һ���û�
	bool DB_SetNewPwd(CString operator_id, CString new_pwd);                 //�޸��û�����
	bool DB_ChangeState(CString op_id, CString state);                       //�ı��û�״̬ state ��login logout
	bool DB_UpdateImageOp(int image_id, CString op_id);                      //����ĳ��ͼƬ�������޸�ʱ������ͼƬ������id
	bool DB_DelOperotors(std::vector<string> ops);                           //ɾ��ָ��ID���û�
	bool DB_AddProperty(int type, CString pro_name);                         //��Figures������ֶΣ��ֶ�����Ĭ��Ϊtext

	/*
	���ݿ��ѯ����
	*/
	int  DB_ImageExisted(CString imagePath);                                 //�жϵ�ǰͼƬ�Ƿ������ݿ��д���������ݣ����ڵĻ�������image_id���������򷵻�0
	std::vector<CString> DB_GetImagePath(CString event_date_start, 
		 CString event_date_end, CString operator_id);                       //���ݲ�������������֮ǰ�����ͼƬ
	string DB_GetFigureCreateSql();                                          //��ȡFigure�������
	map<string, string, greater<string>> DB_GetPropertyValue(int figure_id); //��������id��ȡ�����Լ����Զ�Ӧ��ֵ
	int DB_GetFigureAmount();                                                //��ȡ��ǰFigures�����ѷ���id�����ֵ
	int DB_GetImageAmount();                                                 //��ȡ��ǰImages�����ѷ���id�����ֵ
	std::vector<int> DB_GetFigures(int image_id);                            //image_id��Ӧ������figures��ID
	std::vector<int> DB_GetRects(int figure_id);                             //��ȡfigure_id��Ӧ���������ο��ֵ����12��integerֵ��

	/*
	���ݿ������²���
	*/
	bool DB_WriteData();
	void DB_ReadData(CString imagePath);
	void DB_UpdateData();
	
	/*
	���ݿ��¼ɾ������
	*/
	bool DB_DelCurrentImage(int image_id);                                   //ɾ����ǰͼƬ��Ϣ���Լ�ͼƬ��������Ϣ
	
	//���ߺ���
public:
	std::vector<string> split(string str, string separator);                 //�ַ�������
	string PathToDB(CString imagePath);                                      //��ȡ·���ַ��������������,���������ݿ�file_path�ֶ�
	void Merge(CString imagePath);                                           //���������ͼƬ���е�ָ���ļ���
	CString Encrypt(CString S, WORD Key);                                    //�ַ������ܽ���
	CString Decrypt(CString S, WORD Key);
	CString SetSavePath();                                                   //ָ������ͼƬ·���ĶԻ���
	string GetNearest(cv::Scalar scl, 
		              std::map<string, cv::Scalar> color_hvalue);            //�Ƚ���ɫ���ƶȣ�������ӽ�����ɫ
	CString GetRgbValue(CPoint pt);                                          //����ͼ��ĳһ���rgbֵ������ӽ�����ɫ
	void Lock();
	void UnLock();
	void ConnectServer();                                                    //���ӷ����������������
	WCHAR *mbcsToUnicode(const char *zFilename);
	char *unicodeToUtf8(const WCHAR *zWideFilename);
};
