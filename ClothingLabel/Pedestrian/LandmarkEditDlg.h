
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
	
	//控件处理及消息处理函数
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

	//成员变量
public:
	std::map<int, CRect> db_rect;                                            //figure_id --- 全身矩形框
	std::map<int, CRect> db_upper_rect;                                      //figure_id --- 上半身矩形框
	std::map<int, CRect> db_lower_rect;                                      //figure_id --- 下半身矩形框
	std::vector<CString> Images;                                             //文件夹中剩余的所有图片
	map<int, map<string, string, greater<string>>> db_pro_values;
	CListCtrl m_listCtrl;                                                    //查询结果列表
	CDateTimeCtrl m_dateStart;                                               //查询起始日期
	CDateTimeCtrl m_dateEnd;                                                 //查询终止日期

	//全局、成员变量更新及画图函数
public:
	void ShowImage(CString imagePath);                                       //显示当前处理的图片                             
	void DrawOnBuffer();                                                     //双缓冲绘图
	void SaveOption();                                                       //保存操作
	void DrawRectangle(CRect rect, COLORREF color);                          //在内存绘制矩形
	void DrawRectangle(CPoint X, CPoint Y);                                  //在内存绘制矩形
	void DrawEclipse(int x, int y, int r, COLORREF color);
	void DrawLine(int nPenStyle, CPoint x, CPoint y, COLORREF color);
	void DrawResizePoints();                                                 //在内存绘制可挪动的点

	std::map<string, COLORREF> InitColorRGB();                               //初始化颜色对应的rgb值--用于绘制颜色下拉框
	std::map<string, cv::Scalar> InitColorScalar();                          //初始化颜色对应的scalar值--用于比较颜色相似度
	void InitProValues();                        //获取所有属性可选的属性值
	std::vector<string> GetFigureFieldName(string res);                      //获取figures表中所有可编辑属性
	int GetCurFigureId(CPoint x);                                            //根据鼠标右击位置获取人物id
	int GetCurrentFigure(std::map<int, CRect> rect, CPoint x, CPoint y);  //根据矩形框的位置获取当前人物id
	std::vector<CString> GetRemainingImages(CString basePath);               //获取当前处理路径中的所有图片名称保存在vector中
	void InitRects(int image_id);                                            //根据image_id获取到人体三个矩形框

	//数据库相关操作
public:
	/*
	用户管理相关函数
	*/
	bool DB_AllowLogin(CString operator_id, CString password);               //是否登录成功
	bool DB_OperatorExisted();                                               //operator表中是否存在记录
	bool DB_IsAdmin(CString operator_id);                                    //判断当前用户是否为管理员
	CString DB_GetPwd(CString operator_id);                                  //获取当前输入用户id的密码
	std::map<string, string> DB_GetOperators();                              //获取除管理员外的所有用户id和姓名
	bool DB_InsertOperator(CString operator_id, CString op_name,
		CString password, CString permission, CString state);                //插入一个用户
	bool DB_SetNewPwd(CString operator_id, CString new_pwd);                 //修改用户密码
	bool DB_ChangeState(CString op_id, CString state);                       //改变用户状态 state ：login logout
	bool DB_UpdateImageOp(int image_id, CString op_id);                      //当对某张图片进行了修改时，更新图片操作人id
	bool DB_DelOperotors(std::vector<string> ops);                           //删除指定ID的用户
	bool DB_AddProperty(int type, CString pro_name);                         //给Figures表添加字段，字段类型默认为text

	/*
	数据库查询函数
	*/
	int  DB_ImageExisted(CString imagePath);                                 //判断当前图片是否在数据库中存有相关数据，存在的话，返回image_id，不存在则返回0
	std::vector<CString> DB_GetImagePath(CString event_date_start, 
		 CString event_date_end, CString operator_id);                       //根据操作日期来搜索之前处理的图片
	string DB_GetFigureCreateSql();                                          //获取Figure表创建语句
	map<string, string, greater<string>> DB_GetPropertyValue(int figure_id); //根据人物id获取属性以及属性对应的值
	int DB_GetFigureAmount();                                                //获取当前Figures表中已分配id的最大值
	int DB_GetImageAmount();                                                 //获取当前Images表中已分配id的最大值
	std::vector<int> DB_GetFigures(int image_id);                            //image_id对应的所有figures的ID
	std::vector<int> DB_GetRects(int figure_id);                             //获取figure_id对应的三个矩形框的值（共12个integer值）

	/*
	数据库插入更新操作
	*/
	bool DB_WriteData();
	void DB_ReadData(CString imagePath);
	void DB_UpdateData();
	
	/*
	数据库记录删除操作
	*/
	bool DB_DelCurrentImage(int image_id);                                   //删除当前图片信息，以及图片中人脸信息
	
	//工具函数
public:
	std::vector<string> split(string str, string separator);                 //字符串划分
	string PathToDB(CString imagePath);                                      //获取路径字符串的最后两部分,保存至数据库file_path字段
	void Merge(CString imagePath);                                           //将处理过的图片剪切到指定文件夹
	CString Encrypt(CString S, WORD Key);                                    //字符串加密解密
	CString Decrypt(CString S, WORD Key);
	CString SetSavePath();                                                   //指定保存图片路径的对话框
	string GetNearest(cv::Scalar scl, 
		              std::map<string, cv::Scalar> color_hvalue);            //比较颜色相似度，返回最接近的颜色
	CString GetRgbValue(CPoint pt);                                          //根据图中某一点的rgb值返回最接近的颜色
	void Lock();
	void UnLock();
	void ConnectServer();                                                    //连接服务器，并与服务器
	WCHAR *mbcsToUnicode(const char *zFilename);
	char *unicodeToUtf8(const WCHAR *zWideFilename);
};
