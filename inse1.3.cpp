#include <mysql/mysql.h>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

 #define HOST	"113.251.223.105"
 #define USERNAME "root"
 #define PASSWORD	"274934"
 #define DATABASE	"xxy"
 
 
 int find(const char* source/*源串*/, char* target/*目标串*/);
 int getprinum(int rows);
 int getsubnum(int rows);
 void pri_inse(char *qn,int id,double x,double y);
 void sub_inse(char *qn,int id,double x,double y);
 void page_inse(int rows,const char* pagename,int prinum,int subnum);
 void savesite(tesseract::ResultIterator* ri, char  qn1[][10],char  qn2[][10],tesseract::PageIteratorLevel level,int rows,int height,int width);
 char* my_strcat_pri(int id,char *qn,double x, double y);
 char* my_strcat_sub(int id,char *qn,double x, double y);
 
 int main(int argc,char** argv)
 {
     
    MYSQL mysql;
    MYSQL_RES       *res = NULL;  
    char   *query_str = (char*)malloc(sizeof(char)*18);
    int             rc;
    int             rows;  
    int             prinum,subnum;
    char qn1[10][10] = { "一、","二、","三、","四、","五、","六、","七、","八、","九、","十、"};  
    char qn2[10][10] = { "1.","2.","3.","4.","5.","6.","7.","8.","9.","10."};
    const char* jpgsrc= (const char*)argv[1];
    // printf("%d\n",argc);  
    // printf("%s\n",jpgsrc);
    // exit(0);

    Pix *image = pixRead(jpgsrc);
    int width = pixGetWidth(image);
    int height = pixGetHeight(image);
    tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
    api->Init(NULL, "chi_sim");
    api->SetImage(image);
    api->Recognize(0);
    tesseract::ResultIterator* ri = api->GetIterator();
    tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
    
    stpcpy(query_str, "select * from page");
    mysql_init(&mysql);
    if(!mysql_real_connect(&mysql, HOST, USERNAME, PASSWORD, DATABASE, 0, NULL, 0))
    {
        printf("Connection failed,%s\n",mysql_error(&mysql));
    }
    mysql_query(&mysql, "set names utf8");
     rc = mysql_real_query(&mysql,query_str,strlen(query_str));        
      if (0 != rc) {  
        printf("mysql_real_query(): %s\n", mysql_error(&mysql));  
        return -1;  
    }  
     res = mysql_store_result(&mysql);  
     if (NULL == res) {  
         printf("mysql_restore_result(): %s\n", mysql_error(&mysql));  
         return -1;  
    }  
    rows = mysql_num_rows(res);
    savesite(ri,qn1,qn2,level,rows,height,width);
    prinum = getprinum(rows);
    subnum = getsubnum(rows);
    page_inse(rows,jpgsrc,prinum,subnum);
    mysql_close(&mysql);
     return 0;
 }
 
 void savesite(tesseract::ResultIterator* ri, char  qn1[][10],char  qn2[][10],tesseract::PageIteratorLevel level,int rows,int height,int width){
    int id =rows+1;
    if (ri!= 0)//按文字行遍历
    {
        do
        {
            const char* word = ri->GetUTF8Text(level);//word 是每一行的文字内容
            int site1,site2;
            for (int i = 0; i <  10;   i++){
                site1 = find(word,qn1[i]);
                site2 = find(word,qn2[i]);
            if(site1 != 0 && site2 !=0 )
                continue;
            //float conf =    ri->Confidence(level);
            int x1, y1, x2, y2;
            ri->BoundingBox(level, &y1, &x1, &x2, &y2);
            //printf("word: '%s';  \tconf: %.2f; BoundingBox: %d,%d,%d,%d;\n",word, conf, x1, y1, x2, y2);
            double x = (double)x1/height;
            double y = (double)y1/width;
            if(site1==0 && site2 !=0)
            pri_inse(qn1[i],id,x,y);
            else if(site1 !=0 && site2 == 0)
            sub_inse(qn2[i],id,x,y);
            }
            delete[] word;
        } while (ri->Next(level));
    }
}
 
 void page_inse(int rows,const char* pagename,int prinum,int subnum){
     MYSQL mysql;
     int             rc;
     char temp[80];
     char* query_str = (char*)malloc(sizeof(char)*255);
     sprintf(query_str,"insert into page(pageid,pageName,priNum,subNum) values");
     sprintf(temp,"(%d,'%s',%d,%d)", rows+1,pagename,prinum, subnum); 
    // printf("pageinse query_str: %s\n",query_str);
     strcat(query_str,temp);
     printf("pageinse query_str: %s\n",query_str);
     mysql_init(&mysql);
    if(!mysql_real_connect(&mysql, HOST, USERNAME, PASSWORD, DATABASE, 0, NULL, 0))
    {
        printf("Connection failed,%s\n",mysql_error(&mysql));
    }
    mysql_query(&mysql, "set names utf8");
     rc = mysql_real_query(&mysql,query_str,strlen(query_str));        
      if (0 != rc) {  
        printf("mysql_real_query(): %s\n", mysql_error(&mysql));  
    }  
 }
 
 int getprinum(int rows){
    MYSQL mysql;
    MYSQL_RES       *res = NULL;  
    int             rc;
     int id  = rows+1;
     int prinum;
     char temp [2];
     char *query_str = (char *)malloc(sizeof(char)*40);
     sprintf(query_str,"select * from prititle where pageid = ");
     sprintf(temp,"%d",id);
     strcat(query_str,temp);
     
    mysql_init(&mysql);
    if(!mysql_real_connect(&mysql, HOST, USERNAME, PASSWORD, DATABASE, 0, NULL, 0))
    {
        printf("Connection failed,%s\n",mysql_error(&mysql));
    }
    mysql_query(&mysql, "set names utf8");
     rc = mysql_real_query(&mysql,query_str,strlen(query_str));        
      if (0 != rc) {  
        printf("mysql_real_query(): %s\n", mysql_error(&mysql));  
        return -1;  
    }  
     res = mysql_store_result(&mysql);  
     if (NULL == res) {  
         printf("mysql_restore_result(): %s\n", mysql_error(&mysql));  
         return -1;  
    }  
    prinum = mysql_num_rows(res);
     mysql_close(&mysql);
     
     return  prinum;
 }
 
  int getsubnum(int rows){
    MYSQL mysql;
    MYSQL_RES       *res = NULL;  
    int             rc;
     int id  = rows+1;
     int subnum;
     char temp [2];
     char *query_str = (char *)malloc(sizeof(char)*40);
     sprintf(query_str,"select * from subtitle where pageid = ");
     sprintf(temp,"%d",id);
     strcat(query_str,temp);
     
    mysql_init(&mysql);
    if(!mysql_real_connect(&mysql, HOST, USERNAME, PASSWORD, DATABASE, 0, NULL, 0))
    {
        printf("Connection failed,%s\n",mysql_error(&mysql));
    }
    mysql_query(&mysql, "set names utf8");
     rc = mysql_real_query(&mysql,query_str,strlen(query_str));        
      if (0 != rc) {  
        printf("mysql_real_query(): %s\n", mysql_error(&mysql));  
        return -1;  
    }  
     res = mysql_store_result(&mysql);  
     if (NULL == res) {  
         printf("mysql_restore_result(): %s\n", mysql_error(&mysql));  
         return -1;  
    }  
    subnum = mysql_num_rows(res);
     mysql_close(&mysql);
     
     return  subnum;
 }
 
void pri_inse(char *qn,int id,double x,double y){//将一级题号坐标结果写入数据库
    MYSQL mysql;
    int rc;
    char *  query_str1 = my_strcat_pri(id,qn,x, y);
    mysql_init(&mysql);
    if(!mysql_real_connect(&mysql, HOST, USERNAME, PASSWORD, DATABASE, 0, NULL, 0))
    {
        printf("Connection failed(pri_inse):,%s\n",mysql_error(&mysql));
    }
    mysql_query(&mysql, "set names utf8");
    rc = mysql_real_query(&mysql,query_str1,strlen(query_str1));
      if (0 != rc) {  
        printf("(pri_inse):mysql_real_query(): %s\n", mysql_error(&mysql));  
    }  
    mysql_close(&mysql);
}

void sub_inse(char *qn,int id,double x,double y){//将二级题号坐标结果写入数据库
    MYSQL mysql;
    int rc;
    char  *query_str = my_strcat_sub(id,qn,x, y);
    mysql_init(&mysql);
    if(!mysql_real_connect(&mysql, HOST, USERNAME, PASSWORD, DATABASE, 0, NULL, 0))
    {
        printf("Connection failed(sub_inse):,%s\n",mysql_error(&mysql));
    }
    mysql_query(&mysql, "set names utf8");
     rc = mysql_real_query(&mysql,query_str,strlen(query_str));        
      if (0 != rc) {  
        printf("sub_inse:mysql_real_query(): %s\n", mysql_error(&mysql));  
    }  
    mysql_close(&mysql);
}

char *my_strcat_pri(int id, char *qn, double x, double y){  //拼接一级题号查询语句
    char temp[30];
    char *str = (char *)malloc(sizeof(char)*80);
    sprintf(str, "insert into prititle (pageid,priTit,pnx,pny) values");
    sprintf(temp,"(%d,'%s',%f,%f)",id,qn,x,y);
    strcat(str,temp);
    //printf("-----cat_pri:%s\n",str);
    return str;

 }

char* my_strcat_sub(int id, char *qn, double x, double y){   //.拼接二级题号查询语句
     char temp[30];
    char *str = (char *)malloc(sizeof(char)*80);
    sprintf(str, "insert into subtitle (pageid,subTit,snx,sny) values");
    sprintf(temp,"(%d,'%s',%f,%f)",id,qn,x,y);
    strcat(str,temp);
    //printf("----cat_sub:%s\n",str);
    return str;
 }

int find(const char* source/*源串*/, char* target/*目标串*/)//找到返回位置,未找到返回-1
{
	int i, j;
	int s_len = strlen(source);
	int t_len = strlen(target);
	if (t_len > s_len)
	{
		return -1;
	}
	for (i = 0; i <= s_len - t_len; i++)
	{
		j = 0;
		int flag = 1;
		if (source[i] == target[j])
		{
			int k, p = i;
			for (k = 0; k < t_len; k++)
			{
				if (source[p] == target[j])
				{
					p++;
					j++;
					continue;
				}
				else
				{
					flag = 0;
					break;
				}
			}
		}
		else
		{
			continue;
		}
		if (flag == 1)
		{
			return i;
		}
	}
	return -1;
}
