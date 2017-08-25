
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
	
	//控件处理及消息处理函数
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

	//相关中间处理及画图函数
public:
	std::map<int, std::vector<CPoint>> db_landmark;                          //face_id --- 关键点
	std::map<int, std::map<string, string>> db_pro_values;
	std::map<int, CRect> db_rect;                                            //face_id --- 人脸矩形框
	std::vector<CString> Images;
	shape_predictor GetSP();
	void ShowImage(CString imagePath);                                       //显示当前处理的图片
	void GetInitialLandmark();                                               //获取图像所有人脸的landmark
	void DrawOnBuffer();                                                     //双缓冲绘图
	void DrawEclipse(int x, int y, int r, COLORREF color);                   //在内存绘制圆形
	void DrawLine(CPoint X, CPoint Y);                                       //在内存绘制线段
	void DrawRectangle(CPoint X, CPoint Y);                                  //在内存绘制矩形
	std::vector<CPoint> GetLandmark(CPoint start, CPoint end);               //获取自定义矩形框的landmark
	int GetEditFaceId(CPoint pt);                                            //根据点的位置获取当前处理的人脸id
	std::vector<CString> GetRemainingImages(CString basePath);               //获取当前处理路径中的所有图片名称保存在vector中
	void SaveOption();                                                       //保存操作
	void GetProNameAndValues(std::vector<string> & pro_name, std::vector<string> &profileFieldName,
		std::map<string, std::vector<string>> & landmark_pro_values,
		std::map<string, std::vector<string>> & profile_pro_values);
	//数据库相关操作
public:

	/*
	读取数据库数据操作
	*/
	int  DB_ImageExisted(CString imagePath);                                 //判断当前图片是否在数据库中存有相关数据，存在的话，返回image_id，不存在则返回0
	std::vector<string> DB_GetKeyPointFieldName(string res);                 //获取保存landmark信息的字段名0-135
	std::vector<int> DB_GetFaceID(int image_id);                             //根据image_id获取所有的face_id保存到数组里面
	std::vector<CPoint> DB_GetPoints(int face_id);                           //根据face_id,获取所有landmark点的位置
	CRect DB_GetRect(std::vector<CPoint> points);                            //根据人脸关键点位置，获取人脸矩形框
	std::vector<CString> DB_GetImagePath(CString event_date_start, 
		 CString event_date_end, CString operator_id);                       //根据操作日期来搜索之前处理的图片
	bool DB_AllowLogin(CString operator_id, CString password);               //是否登录成功
	bool DB_OperatorExisted();                                               //operator表中是否存在记录
	bool DB_IsAdmin(CString operator_id);                                    //判断当前用户是否为管理员
	CString DB_GetPwd(CString operator_id);                                  //获取当前输入用户id的密码
	string DB_GetFacesCreateSql();                                           //获取建表sql语句，用于之后的字段名获取
	std::map<string, string> DB_GetOperators();                              //获取除管理员外的所有用户id和姓名
	std::map<string, string> DB_GetProperties(int face_id);                  //根据face_id获取可编辑属性的值保存在vector中
	void DB_ReadData(CString imagePath);

	/*
	数据库写入操作
	*/
	bool DB_InsertOperator(CString operator_id, CString op_name, 
		 CString password, CString permission, CString state);               //插入一个用户
	bool DB_AddProperty(CString pro_name);                                   //给Faces表添加字段，字段类型默认为text
	void DB_WriteData();
	void DB_UpdateData();
	
	/*
	数据库更新操作
	*/
	bool DB_SetNewPwd(CString operator_id, CString new_pwd);                 //修改用户密码
	bool DB_ChangeState(CString op_id, CString state);                       //改变用户状态 state ：login logout
	bool DB_UpdateImageOp(int image_id,CString op_id);                       //当对某张图片进行了修改时，更新图片操作人id


	bool DB_DelCurrentImage(int image_id);                                   //删除当前图片信息，以及图片中人脸信息
	bool DB_DelOperotors(std::vector<string> ops);                           //删除指定ID的用户

	//工具函数
public:
	std::vector<string> split(string str, string separator);                 //字符串划分
	string PathToDB(CString imagePath);                                      //获取路径字符串的最后两部分,保存至数据库file_path字段
	void Merge(CString imagePath);                                           //将处理过的图片剪切到指定文件夹
	CString Encrypt(CString S, WORD Key);                                    //字符串加密解密
	CString Decrypt(CString S, WORD Key);
	std::string encode(const std::string to_encode);
	std::string decode(const std::string to_decode);
	CString SetSavePath();                                                   //指定保存图片路径的对话框
	void ConnectServer();
	WCHAR *mbcsToUnicode(const char *zFilename);
	char *unicodeToUtf8(const WCHAR *zWideFilename);
};
