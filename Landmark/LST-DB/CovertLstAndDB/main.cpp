#pragma comment (lib, "sqlite3.lib")

#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <fstream>
#include <map>
#include <opencv2/core/core.hpp>  
#include <opencv2/highgui/highgui.hpp>  
using namespace std;

extern "C"
{
#include "sqlite3.h"
};

std::string encode(const std::string to_encode)
{
	std::string res = to_encode;
	std::string key = "4962873";
	for (int i = 0, j = 0; res[j]; j++, i = (i + 1) % 7){
		res[j] += key[i];
		if (res[j] > 122) res[j] -= 90;
	}
	return res;
}

std::vector<string> split(string str, string separator)
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

class LstToDB
{
public:
	LstToDB(string lst_path)
	{
		this->lst_path = lst_path;
		parent = lst_path.substr(0, lst_path.find_last_of('\\'));
		cout << parent << endl;
		db_path = parent + "\\landmark.sqlite";
		ReadLabelsFromLst();
		fstream _file;
		_file.open(db_path.c_str());
		if (!_file)
		{
			CreateDBAndTables();
		}
		InsertToDB();
	}
private:
	void ReadLabelsFromLst()
	{
		ifstream read(lst_path);
		string line;
		while (getline(read, line))
		{
			vector<string> temp = split(line, "\t");
			int len = temp.size();
			string path = temp[len - 1];
			vector<float> pts;
			for (int i = 1; i < len - 1; ++i){
				pts.push_back(atof(temp[i].c_str()));
			}
			landmark_num = (len - 2) / 2;
			label l;
			l.path = path;
			l.points = pts;
			labels.push_back(l);
		}
	}
	
	void CreateDBAndTables()
	{
		ofstream out(db_path.c_str());
		out.close();
		int rc = sqlite3_open(db_path.c_str(), &db);
		if (rc == SQLITE_OK)
		{
			//create table "Operator"
			string sql = "CREATE TABLE \"Operators\" (\"operator_id\" INTEGER PRIMARY KEY  AUTOINCREMENT  NOT NULL , \"password\" TEXT NOT NULL , permission text, \"name\" text, \"state\" text)";
			rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
			if (rc == SQLITE_OK)
			{
				string opid = "1";
				string pa = encode("123456");
				string pe = "admin";
				string opn = "admin";
				string stat = "logout";
				sql = "insert into Operators values ('" + opid + "', '" + pa + "', '" + pe + "','" + opn + "','" + stat + "')";
				rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
				if (rc == SQLITE_OK)
				{
					cout << "insert operator 1 successfully!" << endl;
				}
			}

			//create table "Images"
			sql = "CREATE TABLE \"Images\" (\"image_id\" INTEGER PRIMARY KEY  AUTOINCREMENT  NOT NULL , \"file_path\" TEXT NOT NULL , \"width\" INTEGER NOT NULL , \"height\" INTEGER NOT NULL , \"event_date\" TEXT NOT NULL , \"operator_id\" INTEGER)";
			rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
			if (rc == SQLITE_OK)
			{
				cout << "create table Images successfully!" << endl;
			}

			//create table "Faces"
			sql = "CREATE TABLE \"Faces\" (\"face_id\" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL , \"image_id\" integer NOT NULL,\"lm01_x\" integer, \"lm01_y\" integer";
			for (int i = 2; i <= landmark_num; ++i)
			{
				char str[50];
				sprintf(str, ",\"lm%02d_x\" integer, \"lm%02d_y\" integer ", i, i);
				sql += str;
			}
			sql += ")";
			rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
			if (rc == SQLITE_OK)
			{
				cout << "create table Faces successfully!" << endl;
			}
		}
		sqlite3_close(db);
	}

	int get_image_id(string file_path)
	{
		int image_id;
		string sql = "select image_id from Images where file_path='" + file_path + "'";
		
		char** pResult;
		int nRow;
		int nCol;
		int rc = sqlite3_get_table(db, sql.c_str(), &pResult, &nRow, &nCol, NULL);
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

	void InsertToDB()
	{
		int rc = sqlite3_open(db_path.c_str(), &db);
		if (rc == SQLITE_OK)
		{
			cout << labels.size() << endl;
			for (int i = 0; i < labels.size(); ++i)
			{
				string file_path = labels[i].path;
				vector<float> pts = labels[i].points;
				vector<string> temp = split(file_path, "/");
				file_path = temp[0] + "\\" + temp[1];
				int image_id = get_image_id(file_path);
				string image_path = parent + "\\" + file_path;
				cout << image_path << endl;
				cv::Mat m = cv::imread(image_path);
				int im_width = m.cols;
				int im_height = m.rows;
				
				if (image_id == 0)
				{
					//insert record into "Images"
					char sql_insert_image[256];
					sprintf(sql_insert_image, "insert into Images values(NULL, '%s', '%d', '%d' , date('now'), '%d')", file_path.c_str(), im_width, im_height, 1);
					cout << sql_insert_image << endl;
					rc = sqlite3_exec(db, sql_insert_image, NULL, NULL, NULL);
					if (rc == 0)
					{
						cout << "insert into Images successfully!" << endl;
					}
					//get current image id
					char *sql_select = "select seq from sqlite_sequence where name='Images'";
					char** pResult;
					int nRow;
					int nCol;
					rc = sqlite3_get_table(db, sql_select, &pResult, &nRow, &nCol, NULL);
					if (rc == SQLITE_OK)
					{
						image_id = atoi(pResult[nCol]);
					}
				}
				//insert record into "Faces"
				stringstream ss;
				string sql_insert_faces;
				ss << image_id;
				sql_insert_faces = "insert into Faces values(NULL," + ss.str();
				ss.str("");

				for (int i = 0; i < pts.size(); i=i+2)
				{
					float x = pts[i] * im_width;
					float y = pts[i + 1] * im_height;
					ss << "," << x << ", " << y;
					sql_insert_faces += ss.str();
					ss.str("");
				}
				sql_insert_faces += ")";
				cout << sql_insert_faces << endl;
				rc = sqlite3_exec(db, sql_insert_faces.c_str(), NULL, NULL, NULL);
				if (rc == 0)
				{
					cout << "insert into Faces successfully!" << endl;
				}
			}
		}
		sqlite3_close(db);
	}

	string db_path;
	string lst_path;
	string parent;
	sqlite3 *db;
	int landmark_num;
	struct label
	{
		string path;
		vector<float> points;
	};
	std::vector<label> labels;
};

class DBToLst
{
public:
	DBToLst(string db_path)
	{
		this->db_path = db_path;
		parent = db_path.substr(0, db_path.find_last_of('\\'));
		get_landmark_num();
		get_labels();
		write_to_lst();
	}
private:
	std::map<int, string> get_images()
	{
		int rc = sqlite3_open(db_path.c_str(), &db);
		std::map<int, string> res;
		string sql = "select image_id, file_path from Images";
		char** pResult;
		int nRow;
		int nCol;
		rc = sqlite3_get_table(db, sql.c_str(), &pResult, &nRow, &nCol, NULL);
		if (rc == SQLITE_OK)
		{
			//cout << nRow << " " << nCol << endl;
			if (nRow >= 1)
			{
				for (int i = nCol; i < (nRow + 1)*nCol; i = i + 2)
				{
					int image_id = atoi(pResult[i]);
					string file_path = (pResult[i + 1]);
					res[image_id] = file_path;
					//cout << image_id << ":" << file_path << endl;
				}
			}
		}
		sqlite3_free_table(pResult);
		//cout << res.size() << endl;
		sqlite3_close(db);
		return res;
	}

	string GetFacesCreateSql()
	{
		string res = "";
		int rc = sqlite3_open(db_path.c_str(), &db);
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

	void get_landmark_num()
	{
		string sql = GetFacesCreateSql();
		std::vector<string> r = split(sql, ",");
		landmark_num = (r.size() - 2) / 2;
	}

	void get_labels()
	{
		std::map<int, string> Images = get_images();
		std::map<int, string>::iterator it;
		int rc = sqlite3_open(db_path.c_str(), &db);
		if (rc == SQLITE_OK)
		{
			for (it = Images.begin(); it != Images.end(); ++it)
			{
				int image_id = it->first;
				stringstream ss;
				ss << image_id;
				string file_path = it->second;
				string image_path = parent + "\\" + file_path;
				vector<string> tmp_path = split(file_path, "\\");
				string write_path = tmp_path[0] + "/" + tmp_path[1];
				cv::Mat img = cv::imread(image_path);
				int im_w = img.cols;
				int im_h = img.rows;
				string sql = "select lm01_x, lm01_y";
				for (int i = 2; i <= landmark_num; ++i)
				{
					char temp[30];
					sprintf(temp,", lm%02d_x, lm%02d_y", i, i);
					sql += temp;
				}
				sql += " from Faces where image_id=" + ss.str();
				ss.str("");
				//cout << sql << endl;
				char** pResult;
				int nRow;
				int nCol;
				int rc = sqlite3_get_table(db, sql.c_str(), &pResult, &nRow, &nCol, NULL);
				if (rc == SQLITE_OK)
				{
					//cout << nCol << " " << nRow << endl;
					for (int i = nCol; i < (nRow + 1)*nCol; i = i + landmark_num * 2)
					{
						vector<float> pts;
						for (int j = 0; j < landmark_num * 2; j=j+2)
						{
							float x = atof(pResult[i + j]);
							float y = atof(pResult[i + j + 1]);
							//cout << x << " " << y << " ";
							x /= im_w;
							y /= im_h;
							//cout << x << " " << y << " ";
							pts.push_back(x);
							pts.push_back(y);
						}
						//cout << endl;
						//cout << pts.size() << endl;
						label l;
						l.path = write_path;
						l.points = pts;
						labels.push_back(l);
						/*
						for (int k = 0; k < pts.size(); ++k)
						{
							cout << pts[k] << " ";
						}
						cout << endl;*/
						pts.clear();
					}
				}
			}
			cout << "read db done!" << endl;
			//cout << labels.size() << endl;
		}
		sqlite3_close(db);
	}

	void write_to_lst()
	{
		string Lst = parent + "\\labels.lst";
		ofstream out;
		out.open(Lst.c_str());
		cout << labels.size() << endl;
		for (int i = 0; i < labels.size(); ++i)
		{
			out << i << "\t";
			string path = labels[i].path;
			vector<float> pts = labels[i].points;
			cout << pts.size() << endl;
			for (int j = 0; j < pts.size(); ++j)
			{
				out << pts[j] << "\t";
			}
			out << path << endl;
		}
		out.close();
	}


	sqlite3 *db;
	string lst_path;
	string parent;
	string db_path;
	int landmark_num;
	struct label
	{
		string path;
		vector<float> points;
	};
	std::vector<label> labels;
};


static const char help[] = "Conversion between Lst and DB\n"
"0:   convert Lst to landmark.sqlite, like \"CovertLstAndDB.exe 0 LstPath\"\n"
"1:   convert landmark.sqlite to Lst, like \"CovertLstAndDB.exe 1 DBPath\"\n\n"
"Attention: \n"
"(1)Lst file, sqlite file and image directories should be stored in the same file path\n"
"(2)Path should be absolute file path\n\n";

int main(int argc, char* argv[])
{
	//cout << argc << endl;
	if (argc != 3) 
	{
		cout << help;
	}
	else if (strcmp(argv[1], "0") == 0){
		string lst_path = argv[2];
		LstToDB test(lst_path);
	}
	else if (strcmp(argv[1], "1") == 0){
		string db_path = argv[2];
		DBToLst test(db_path);
	}
	return 0;
}

