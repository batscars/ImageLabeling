
// LandmarkEditDlg.h : header file

#pragma once
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <string>
#include <iostream>
#include <opencv2/core/core.hpp>  
#include <opencv2/highgui/highgui.hpp>  
extern "C"
{
  #include "sqlite3.h"
};
using namespace dlib;
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
	afx_msg void OnNewLandmark();
	afx_msg void OnOpenPic();
	afx_msg void OnEditLm();
	afx_msg void OnEditPro();
	afx_msg void OnEditProfile();
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
	afx_msg void OnLandmarktask();
	afx_msg void Onprofiletask();
	afx_msg void OnReadFaces();

	//����м䴦����ͼ����
public:
	std::map<int, std::vector<CPoint>> db_landmark;                          //face_id --- �ؼ���
	std::map<int, std::map<string, string>> db_pro_values;
	std::map<int, CRect> db_rect;                                            //face_id --- �������ο�
	std::vector<CString> Images;
	shape_predictor GetSP();
	void ShowImage(CString imagePath);                                       //��ʾ��ǰ�����ͼƬ
	void GetInitialLandmark();                                               //��ȡͼ������������landmark
	void DrawOnBuffer();                                                     //˫�����ͼ
	void DrawEclipse(int x, int y, int r, COLORREF color);                   //���ڴ����Բ��
	void DrawLine(CPoint X, CPoint Y);                                       //���ڴ�����߶�
	void DrawRectangle(CPoint X, CPoint Y);                                  //���ڴ���ƾ���
	std::vector<CPoint> GetLandmark(CPoint start, CPoint end);               //��ȡ�Զ�����ο��landmark
	int GetEditFaceId(CPoint pt);                                            //���ݵ��λ�û�ȡ��ǰ���������id
	std::vector<CString> GetRemainingImages(CString basePath);               //��ȡ��ǰ����·���е�����ͼƬ���Ʊ�����vector��
	void SaveOption();                                                       //�������
	void GetProNameAndValues(std::vector<string> & pro_name, std::vector<string> &profileFieldName,
		std::map<string, std::vector<string>> & landmark_pro_values,
		std::map<string, std::vector<string>> & profile_pro_values);
	//���ݿ���ز���
public:

	/*
	��ȡ���ݿ����ݲ���
	*/
	int  DB_ImageExisted(CString imagePath);                                 //�жϵ�ǰͼƬ�Ƿ������ݿ��д���������ݣ����ڵĻ�������image_id���������򷵻�0
	std::vector<string> DB_GetKeyPointFieldName(string res);                 //��ȡ����landmark��Ϣ���ֶ���0-135
	std::vector<int> DB_GetFaceID(int image_id);                             //����image_id��ȡ���е�face_id���浽��������
	std::vector<CPoint> DB_GetPoints(int face_id);                           //����face_id,��ȡ����landmark���λ��
	CRect DB_GetRect(std::vector<CPoint> points);                            //���������ؼ���λ�ã���ȡ�������ο�
	std::vector<CString> DB_GetImagePath(CString event_date_start, 
		 CString event_date_end, CString operator_id);                       //���ݲ�������������֮ǰ�����ͼƬ
	bool DB_AllowLogin(CString operator_id, CString password);               //�Ƿ��¼�ɹ�
	bool DB_OperatorExisted();                                               //operator�����Ƿ���ڼ�¼
	bool DB_IsAdmin(CString operator_id);                                    //�жϵ�ǰ�û��Ƿ�Ϊ����Ա
	CString DB_GetPwd(CString operator_id);                                  //��ȡ��ǰ�����û�id������
	string DB_GetFacesCreateSql();                                           //��ȡ����sql��䣬����֮����ֶ�����ȡ
	std::map<string, string> DB_GetOperators();                              //��ȡ������Ա��������û�id������
	std::map<string, string> DB_GetProperties(int face_id);                  //����face_id��ȡ�ɱ༭���Ե�ֵ������vector��
	void DB_ReadData(CString imagePath);

	/*
	���ݿ�д�����
	*/
	bool DB_InsertOperator(CString operator_id, CString op_name, 
		 CString password, CString permission, CString state);               //����һ���û�
	bool DB_AddProperty(CString pro_name);                                   //��Faces������ֶΣ��ֶ�����Ĭ��Ϊtext
	void DB_WriteData();
	void DB_UpdateData();
	
	/*
	���ݿ���²���
	*/
	bool DB_SetNewPwd(CString operator_id, CString new_pwd);                 //�޸��û�����
	bool DB_ChangeState(CString op_id, CString state);                       //�ı��û�״̬ state ��login logout
	bool DB_UpdateImageOp(int image_id,CString op_id);                       //����ĳ��ͼƬ�������޸�ʱ������ͼƬ������id


	bool DB_DelCurrentImage(int image_id);                                   //ɾ����ǰͼƬ��Ϣ���Լ�ͼƬ��������Ϣ
	bool DB_DelOperotors(std::vector<string> ops);                           //ɾ��ָ��ID���û�

	//���ߺ���
public:
	std::vector<string> split(string str, string separator);                 //�ַ�������
	string PathToDB(CString imagePath);                                      //��ȡ·���ַ��������������,���������ݿ�file_path�ֶ�
	void Merge(CString imagePath);                                           //���������ͼƬ���е�ָ���ļ���
	CString Encrypt(CString S, WORD Key);                                    //�ַ������ܽ���
	CString Decrypt(CString S, WORD Key);
	std::string encode(const std::string to_encode);
	std::string decode(const std::string to_decode);
	CString SetSavePath();                                                   //ָ������ͼƬ·���ĶԻ���
	void ConnectServer();
	WCHAR *mbcsToUnicode(const char *zFilename);
	char *unicodeToUtf8(const WCHAR *zWideFilename);
};
