#include <mysql/mysql.h>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define HOST     "ip"
#define USERNAME "root"
#define PASSWORD	"pwd"
#define DATABASE	"xxy"

typedef struct  qtnode {
	const char * title;
	double x;
	double y;
	struct qtnode *next;
} qtNode,*QuesList;

QuesList createQuesList();
void print(QuesList L);
bool addqtNode(QuesList L,const char* qn,double x,double y);
void query_status(MYSQL mysql,int rc);
void store_status(MYSQL mysql,MYSQL_RES  *res);
int  get_match(MYSQL mysql,int rows,QuesList prilist,QuesList sublist);
int  get_ques_num(MYSQL mysql,int i);
int  getrows(MYSQL mysql,const char *sql);
int  get_len(QuesList L);
int  sel_comp(MYSQL mysql,const char* sql,QuesList list);
char*sql_cat(const char* table,int i);
int  find(const char* source/*源串*/, char* target/*目标串*/);
void getsite(tesseract::ResultIterator* ri,char qn1[][10],char qn2[][10],tesseract::PageIteratorLevel level,int height,int width,QuesList prilist,QuesList sublist);

int main(int argc,char** argv)
{

	MYSQL        mysql;
	QuesList     prilist,sublist;
	int          id,rows;
	clock_t startTime,endTime;
	char qn1[10][10] = { "一、","二、","三、","四、","五、","六、","七、","八、","九、","十、"};
	char qn2[10][10] = { "1.","2.","3.","4.","5.","6.","7.","8.","9.","10."};
    const char* jpgsrc= (const char*)argv[1];
    //printf("%d\n",argc);  
    //printf("%s\n",jpgsrc);
    //exit(0);
	prilist = createQuesList();
	sublist = createQuesList();

	startTime = clock();
	Pix *image = pixRead(jpgsrc);
	int width = pixGetWidth(image);
	int height = pixGetHeight(image);
	tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
	api->Init(NULL, "chi_sim");
	api->SetImage(image);
	api->Recognize(0);
	tesseract::ResultIterator* ri = api->GetIterator();
	tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
	getsite(ri,qn1,qn2,level,height,width,prilist,sublist);
	endTime = clock();
	printf( "第一阶段读取图片时间：%f seconds\n",(double)(endTime - startTime) / CLOCKS_PER_SEC);
	printf("prilist:\n");
	print(prilist);
	printf("sublist:\n");
	print(sublist);
	mysql_init(&mysql);
	if(!mysql_real_connect(&mysql, HOST, USERNAME, PASSWORD, DATABASE, 0, NULL, 0)) {
		printf("Connection failed,%s\n",mysql_error(&mysql));
	}
	mysql_query(&mysql, "set names utf8");
	rows = getrows(mysql,"select *from page");
	//printf("rows:%d\n",rows);
	startTime = clock();
	id = get_match(mysql,rows,prilist,sublist);
	endTime = clock();
	printf( "第二阶段匹配图片时间：%f seconds\n",(double)(endTime - startTime) / CLOCKS_PER_SEC);
	mysql_close(&mysql);

	return 0;
}

int sel_comp(MYSQL mysql,const char* sql,QuesList list)
{
	//将载入图片的坐标记录进list,与数据库的数据进行比较
	MYSQL_RES   *res = NULL;
	MYSQL_ROW        row;
	int count = 0;
	int rows;
	double dx,dy;
	unsigned int     num_fields;
	unsigned int     rc;
	char   *query_str = (char*)malloc(sizeof(char));
	stpcpy(query_str,sql);
	//printf("%s\n",query_str);
	rc = mysql_real_query(&mysql,query_str,strlen(query_str));
	query_status(mysql,rc);
	res = mysql_store_result(&mysql);
	store_status(mysql,res);
	rows = mysql_num_rows(res);
	num_fields = mysql_num_fields(res);
	while(row = mysql_fetch_row(res)) {
		qtNode *p;
		p = list->next;
		const char *s1 =  (const char*)row[1];
		double x = atof(row[2]);
		double y = atof(row[3]);
		//printf("row[1] = %s\t row[2] = %s\t row[3] = %s\n",row[1],row[2],row[3]);
		//printf("x = %f\t y = %f\n",x,y);
		while(p!=NULL) {
			const char *s2 =  p->title;
			int flag = strcmp(s1,s2);
			//printf("flag = %d\n",flag);
			//printf("row[1] = %s\t p->title = %s\n",row[1],p->title);
			if(flag == 0) {
				//printf("p->x = %f\t p->y = %f\n",p->x,p->y);
				dx  = fabs(x - (p->x));
				dy  = fabs(y - (p->y));
				//printf("dx = %f\t dy = %f\n",dx,dy);
				if(dx<0.08 && dy < 0.08 ) {
					count++;
					//printf("count++ == %d\n",count);
					break;
				}
			}
			p = p->next;
		}
	}
	mysql_free_result(res);
	return count;
}

int get_match(MYSQL mysql,int rows,QuesList prilist,QuesList sublist)
{
	//返回匹配结果
	int count1 = 0,count2 = 0;
    int ques_num = get_len(prilist)+get_len(sublist);
	for(int i = 1; i <= rows; i++) {
		//int ques_num = 0;
		const char* sql1 = sql_cat("prititle",i);
		count1 = sel_comp(mysql,sql1,prilist);
		const char* sql2 = sql_cat("subtitle",i);
		count2 = sel_comp(mysql,sql2,sublist);
		//ques_num = get_len(prilist)+get_len(sublist);//get_ques_num(mysql,i);
		double percent = (double)(count1+count2)/ques_num;
		printf("count1 (%d):%d\n",i,count1);
		printf("count2 (%d):%d\n",i,count2);
		printf("ques_num = %d\n",ques_num);
		printf("------------------------------------------------------\n");
		if (percent > 0.8) {
			printf("对应的图片id是%d\n",i);
			return i;
			break;
		}

	}
	printf("not found\n");
	return 0;
}


int  getrows(MYSQL mysql,const char *sql)
{
	//返回数据库page表记录数
	MYSQL_RES   *res = NULL;
	int rows;
	//MYSQL_ROW        row;
	//unsigned int     num_fields;
	unsigned int     rc;
	char   *query_str = (char*)malloc(sizeof(char));
	stpcpy(query_str,sql);
	//printf("%s\n",query_str);
	rc = mysql_real_query(&mysql,query_str,strlen(query_str));
	query_status(mysql,rc);
	res = mysql_store_result(&mysql);
	store_status(mysql,res);
	rows = mysql_num_rows(res);
	/* num_fields = mysql_num_fields(res);
	while(row = mysql_fetch_row(res)) {
		for(int i=0; i<num_fields; i++) {
			printf("%s\t",row[i]);
		}
		printf("\n");
	} */
	mysql_free_result(res);
	return rows;
}

int get_ques_num(MYSQL mysql,int i)
{
	//返回页面题号特征总数量
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	unsigned int rc;
	unsigned int ques_num;
	const char* query_str = sql_cat("page",i);
	rc = mysql_real_query(&mysql,query_str,strlen(query_str));
	query_status(mysql,rc);
	res = mysql_store_result(&mysql);
	store_status(mysql,res);
	row = mysql_fetch_row(res);
	ques_num = atof(row[1])+atof(row[2]);
	return ques_num;
}
void print(QuesList L)
{
	//打印链表
	qtNode *pTemp;//pTemp为循环所用而临时创建的
	pTemp=L->next;//临时指针指向首结点的地址
	while(pTemp!=NULL) {
		printf("%s\t",pTemp->title);
		printf("%f\t",pTemp->x);
		printf("%f\t",pTemp->y);
		printf("\n");
		pTemp = pTemp->next; 
	}
}

int get_len(QuesList L)   //计算链表的长度
{
    int Len=0;
	qtNode *pTemp;//pTemp为循环所用而临时创建的
	pTemp=L->next;//临时指针指向首结点的地址
	while(pTemp!=NULL) {
		Len++;
		pTemp=pTemp->next;
	}
	return Len;
}

QuesList createQuesList()
{
	//创建链表
	qtNode *L;
	L = (qtNode*) malloc(sizeof(qtNode));
	if(NULL == L) {
		printf("申请内存空间失败/n");
	}
	L->next = NULL;
	return L;
}


void getsite(tesseract::ResultIterator* ri,char qn1[][10],char qn2[][10],tesseract::PageIteratorLevel level,int height,int width,QuesList prilist,QuesList sublist)
{
	//获取载入图片的题号坐标，存入prilist和sublist
	if (ri!= 0) { //按文字行遍历
		do {
			const char* word = ri->GetUTF8Text(level);//word 是每一行的文字内容
			int site1,site2;
			for (int i = 0; i <  10;   i++) {
				site1 = find(word,qn1[i]);
				site2 = find(word,qn2[i]);
				if(site1 != 0 && site2 !=0 )
					continue;
				float conf =    ri->Confidence(level);
				int x1, y1, x2, y2;
				ri->BoundingBox(level, &y1, &x1, &x2, &y2);
				//printf("word: '%s';  \tconf: %.2f; BoundingBox: %d,%d,%d,%d;\n",word, conf, x1, y1, x2, y2);
				double x = (double)x1/height;
				double y = (double)y1/width;
				if(site1==0 && site2 !=0)
					addqtNode(prilist,qn1[i],x,y);
				//printf("insertres(qn1)：%d\n",addqtNode(prilist,qn1[i],x,y));
				else if (site1!=0 && site2 ==0)
					addqtNode(sublist,qn2[i],x,y);
				//printf("insertres(qn2)：%d\n",addqtNode(prilist,qn2[i],x,y));
			}
			delete[] word;
		} while (ri->Next(level));
	}
}

bool addqtNode(QuesList L,const char* qn,double x,double y)
{
	//添加链表节点
	qtNode* node = (qtNode*) malloc(sizeof(qtNode));
	node->title = qn;
	node->x = x;
	node->y = y;
	//尾插法
	if(NULL == L) {
		return false;
	}
	qtNode* p = L->next;
	qtNode* q = L;
	while(NULL != p) {
		q = p;
		p = p->next;
	}
	q->next = node;
	node->next = NULL;
	return true;
}

char* sql_cat(const char* table,int i)
{
	//拼接sql语句
	char *str = (char *)malloc(sizeof(char)*80);
	sprintf(str,"select * from %s where pageid = %d",table,i);
	return str;
}

int find(const char* source/*源串*/, char* target/*目标串*/)//找到返回位置,未找到返回-1
{
	//题号字符串匹配
	int i, j;
	int s_len = strlen(source);
	int t_len = strlen(target);
	if (t_len > s_len) {
		return -1;
	}
	for (i = 0; i <= s_len - t_len; i++) {
		j = 0;
		int flag = 1;
		if (source[i] == target[j]) {
			int k, p = i;
			for (k = 0; k < t_len; k++) {
				if (source[p] == target[j]) {
					p++;
					j++;
					continue;
				} else {
					flag = 0;
					break;
				}
			}
		} else {
			continue;
		}
		if (flag == 1) {
			return i;
		}
	}
	return -1;
}

void query_status(MYSQL mysql,int rc)
{
	//查询状态判断
	if (0 != rc) {
		printf("mysql_real_query(): %s\n", mysql_error(&mysql));
	}
}

void  store_status(MYSQL mysql,MYSQL_RES  *res)
{
	//结果集存储状态判断
	if (NULL == res) {
		printf("mysql_restore_result(): %s\n", mysql_error(&mysql));
	}
}
