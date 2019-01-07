#include "crow.h"
#include <chrono>

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <algorithm>
#include <map>
#include <sstream>
#include <set>
#include <bitset>
#include <math.h>
#include <iterator>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctype.h>
#include <iconv.h>
#include <sys/types.h>
#include <dirent.h>
#include <vector>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <numeric>
#define pathname "./database/"
#define MAXFILENAMEBYTES 120
#define CHECK_LENGTH 20       //检查是否为utf8编码时所检查的字符长度
using namespace std;

typedef struct word2{
	string keyword;
	int off;
	bool operator < (const struct word2 &va) const{
		return keyword<va.keyword;
	}
	bool operator == (const struct word2 &va) const{
		return keyword==va.keyword;
	}
}Word;				//define the type of the elements in the result returned by splitting the search string
typedef list<int> WordOff;//词偏移
typedef struct{
	string sentence;
	int off;
}STR;				//clause
typedef struct VVa{
	string filename;
	int count;//出现的次数
	int total;//文章中的总词数
	WordOff offset;//词偏移
	bool operator < (const struct VVa &va) const{
		return filename<va.filename;
	}
}Va;
typedef struct {
     string name;//文件名
     string article;//文件正文
     string title;
     int flag;//标号
}FILE_S;//保存文件信息的结构体
typedef struct FileScore{
	string filename;
	float score;
	WordOff off;
	WordOff poff;
	bool operator <(Va & fs) const{
		return filename<fs.filename;
	}

	bool operator > (Va &fs)const{
		return filename>fs.filename;
	}
}FS;
typedef struct{
	string abstract;
	string filename;
}ABS;//data type of abstract
class CodeConverter {//the class  is to create a handle
	private:
	iconv_t cd;
	public:
	// 构\E9\80?
	CodeConverter(char *from_charset,char *to_charset) {
	cd = iconv_open(to_charset,from_charset);
	}

	// 析构
	~CodeConverter() {
	iconv_close(cd);
	}

	// 转换输出
	int convert(char *inbuf,int inlen,char *outbuf,int outlen) {
	char **pin = &inbuf;
	char **pout = &outbuf;
	memset(outbuf,0,outlen);
	return iconv(cd,pin,(size_t *)&inlen,pout,(size_t *)&outlen);

	}
};
typedef list <FS> ScoreLocate;
typedef list<Va> WordLocate;//每个词所对应的所以文件信息
typedef map<string,list<Va> >  WordMap;
typedef list <ABS> ABS_LS;//abstract list
typedef list<Word>  DLST;//the result data type of dividing a string
typedef list<STR> STRLIST;
typedef map<string,int> DICTION;//word dictonary

string read();//read punc from punc.txt;
void  judge(string in_str,set<string> &CHAR);// judge chi_punc or eng_punc;
STRLIST split_minor(string str,set<string> CHAR);//split sentences by punc, number or letter;
STRLIST split(string str);//return the sentences list;
/********************/
DLST all_divi_res(STRLIST str,DICTION w_dictionary);//to find all the words which  starts with the character appearing in the search statement,and deal with them
DLST last_choose_res(DLST,string search);//Adopt  one strategy to decide the final consequence of the combination of key words
DLST one_divi_res(STR Str,DICTION w_dictionary);//to divide one string to serveral words
DLST deal_with_res(DLST res,STR str);//to get the splitting result of one substring
DICTION readTxt(const char *s,DICTION temp);//从.txt文件读词库到map
int createfilelist(string path,list <FILE_S> &fileInfo);
void updateMap(WordMap &wordmap,WordMap::iterator it,WordLocate Filelist,Word word,FILE_S file,int totalWord);
void addTomap(WordMap &wordmap,Word word,FILE_S file,int totalWord);
void fileWordMap(WordMap &wordmap,list <FILE_S> fileInfo,DICTION & w_dictionary);
DLST divide(string sentence,DICTION w_dictionary);//to splitsentence to serveral word
void trans_to_string(DLST res,string * keyword,int *keyoff);// DLST type transform to string array;
/**********************************/
//the next two functions  are used to insert helight tab to the abstract string
string STRCAT(string a);
string mode_matct(string in_str,string key);
/**********************************/

ABS get_file_abs(FS file,list <FILE_S> fileInfo,string*keyword,int n);  //to get the abstract of one file
ABS_LS final_abs_list(ScoreLocate l ,list <FILE_S> fileInfo,string *keyword,int n);//to get  abstract  of each element whic is  in the list
/********************/
string codeTransfer(char *in_gb2312); //to check the coding is UTF-8,in_gb2312 points to the input of user;
bool is_utf8_string(char *utf);//to judge the code of string
/********************/
void pwrit_off(WordOff::iterator &f){
	cout<<"itr->off:"<<*f<<endl;
}
void pwrit_locate(WordOff::iterator &f){
	cout<<"locate->off:"<<*f<<endl;
}

void pwrit_Va(ScoreLocate::iterator &v ){
	cout<<"filename:"<<v->filename<<endl;
	cout<<"score:"<<v->score<<endl;
	WordOff::iterator f;
    for(f=v->off.begin();f!=v->off.end();f++)
		pwrit_off(f);
    for(f=v->poff.begin();f!=v->poff.end();f++)
		pwrit_locate(f);
	cout<<endl;
}
void pwrite_off(WordOff::iterator &f){
	cout<<"off:"<<*f<<endl;
}
void pwrite_Va(WordLocate::iterator &v ){
	cout<<"filename:"<<v->filename<<endl;
	cout<<"count:"<<v->count<<endl;
	cout<<"total:"<<v->total<<endl;
	WordOff::iterator f;
    for(f=v->offset.begin();f!=v->offset.end();f++)
		pwrite_off(f);
}
void pwrite_list(WordMap::iterator &i){
	WordLocate::iterator v;
    for(v=i->second.begin();v!=i->second.end();v++)
		pwrite_Va(v);
}
void pwrite_map(WordMap M){
    	WordMap::iterator i;
    	for(i=M.begin();i!=M.end();i++){
    		cout<<"wordname:"<<i->first<<endl;
			pwrite_list(i);
	}
}
bool is_Vs(ifstream &infile){
		char Vs;
		infile.read((char*)&Vs,sizeof(char));
		return Vs=='`';
}

bool is_Os(ifstream &infile){
		char Os;
		infile.read((char*)&Os,sizeof(char));
		return Os=='^';
}

void read_off(ifstream&infile,WordOff &off){
	while(is_Os(infile)){
			int of;
			infile.read((char*)&of,sizeof(int));
			off.push_back(of);
	}
}
void read_Va(ifstream &infile,WordLocate&val){
	Va va;

	char *name=(char*)malloc(MAXFILENAMEBYTES*sizeof(char));
	infile.read((char*)name,MAXFILENAMEBYTES*sizeof(char));

	int count;
	infile.read((char*)&count,sizeof(int));

	int total;
	infile.read((char*)&total,sizeof(int));

	WordOff off;
	read_off(infile,off);
	va={name,count,total,off};
	free(name);
	val.push_back(va);
}
void read_map(WordMap& M,string &word){
	string dir=pathname;
	dir=dir+word;
	ifstream infile(dir,ios::in | ios::binary);
	WordLocate val;
	if(is_Vs(infile))
	do{
		read_Va(infile,val);
		infile.seekg((-1)*sizeof(char),ios::cur);
	}while(is_Vs(infile));
	M[word]=val;
	infile.close();
}
void read_stru(WordMap&M){
		DIR *dp;
		struct dirent *dirp;
		vector<string> file_names;
		if((dp = opendir(pathname)) == NULL){
		perror("Fail to open dir:");
		exit(1);
		}
		while((dirp = readdir(dp)) != NULL){
			if(dirp->d_name[0] != '.'){
			file_names.push_back(dirp->d_name);
			}
		}
		for(vector<string>::iterator it = file_names.begin(); it != file_names.end(); it++){
			read_map(M,*it);
		}
}
void write_off(ofstream &outfile, WordOff::iterator &f){
	char Os='^';
	outfile.write((char *)&Os,sizeof(char));
	outfile.write((char *)&*f,sizeof(int));
}

void write_Va(ofstream &outfile,WordLocate::iterator &v ){
	char Vs='`';
	outfile.write((char *)&Vs,sizeof(char));
	const char*p=v->filename.c_str();
	outfile.write((char *)p,MAXFILENAMEBYTES*sizeof(char));
	outfile.write((char *)&(v->count),sizeof(int));
	outfile.write((char *)&(v->total),sizeof(int));
	WordOff::iterator f;
	for(f=v->offset.begin();f!=v->offset.end();f++){
		write_off(outfile,f);
	}
}
void write_map(WordMap M,WordMap::iterator &i){
	string dir=pathname;
	dir=dir+i->first;
	i->second.sort();
	ofstream outfile(dir,ios::out | ios::trunc|ios::binary);
	WordLocate::iterator v;
	for(v=i->second.begin();v!=i->second.end();v++){
		write_Va(outfile,v);
	}
	outfile.close();
}
void write_data_base(WordMap M){
		for(WordMap::iterator i=M.begin();i!=M.end();i++)
			write_map(M,i);
}
void iniva(WordLocate &val,string filename,int count,int total,WordOff off){
    		Va va={filename,count,total,off};
    		val.push_back(va);
}

void insert(WordMap&M,string&word,string filename[],int n,int count[],int total[],WordOff off[]){
    		WordLocate val;
			for(int i=0;i<n;i++)
				iniva(val,filename[i],count[i],total[i],off[i]);
			val.sort();
			M[word]=val;
}
void cut(ScoreLocate::iterator &first1,ScoreLocate::iterator &last1,WordLocate::iterator &first2,WordLocate::iterator &last2, ScoreLocate &tmp,int ko[],int k){
	ScoreLocate::iterator result=tmp.begin();
	int i=0,j=0;
	while(first1!=last1&&first2!=last2){
		if(first2->count<ko[k]){
			j++;
			cout<<j<<endl;
			first2++;
		}
			if(*first1<*first2){
				first1++;
				i++;
			}
			else if(*first1>*first2){
				first2++;
				j++;
			}
			else {
				FS fs;
				WordOff off;
				off=first1->off;
				off.push_back(j);
				string filename=first1->filename;
				fs={filename,0,off};
				tmp.push_back(fs);

				first1++;
				first2++;
				i++;
				j++;
				result++;
			}
	}
}
void cmp(ScoreLocate &tmp,ScoreLocate &val1,WordLocate &val2,int ko[],int i){
	ScoreLocate::iterator v1b=val1.begin();
	ScoreLocate::iterator v1e=val1.end();
	WordLocate::iterator v2b=val2.begin();
	WordLocate::iterator v2e=val2.end();
	cut(v1b,v1e,v2b,v2e,tmp,ko,i);
}
void cir(WordMap & M,string word[],int ko[],int n,ScoreLocate &tmp,ScoreLocate &d,int i){
	cmp(tmp,d,M[word[i]],ko,i);
	d.clear();
	if(i==n-1){
		return;
	}
	d.splice(d.begin(),tmp);
	tmp.clear();
}
void sw(ScoreLocate &d,WordLocate::iterator v,int i){
	FS fs;
	WordOff off;
	WordOff poff;
	off.push_back(i);
	fs={v->filename,0,off,poff};
	d.push_back(fs);
}
void inisc(ScoreLocate &d,WordLocate&val,int ko[]){
	int i=0;
	for(WordLocate::iterator v=val.begin();v!=val.end();v++){
		if(v->count>=ko[0])
			sw(d,v,i);
		i++;
	}
}
void combine(ScoreLocate &tmp,WordMap M,string word[],int ko[],int n){
	ScoreLocate d;
	inisc(d,M[word[0]],ko);

	/*ScoreLocate::iterator v;
	for(v=d.begin();v!=d.end();v++)
		pwrit_Va(v);
		*/

	if(n==1){
		tmp=d;
	}
	for(int i=1;i<n;i++)
		cir(M,word,ko,n,tmp,d,i);

	//打印归一合并
	/*ScoreLocate::iterator v;
	for(v=tmp.begin();v!=tmp.end();v++)
		pwrit_Va(v);
		*/

}
typedef struct{
	int count;
	int total;
}W;
typedef list<W> VS;
typedef struct{
	int count;
	int total;
	int off;
}WF;
typedef multimap <string,WF> MMap;
MMap::iterator pre_i(MMap&c,string word){
	MMap::iterator w0=c.equal_range(word).first;
	return w0;
}

float cal_var_vec(WordOff v,VS vs){
		float sum =accumulate(v.begin(),v.end(), 0.0);
		float mean =  sum / v.size();
		float accum  = 0.0;
		float sumlog  = 0.0;
		VS::iterator s=vs.begin();
		WordOff::iterator t=v.begin();
		for(s,t;t!=v.end()&&s!=vs.end();t++,s++){
			float logver=(float)(log10(s->total/s->count));
			 accum +=logver*(*t-mean)*(*t-mean);
			 sumlog+=logver;
		}
		return (accum/sumlog);
}
MMap::iterator pan(MMap&c,string word[],int cur,int n){
	if(cur==n-1){
			return c.end();
	}
	else {
			int i;
			int f=1;
			for(i=cur;i<n-1;i++)
			//if(pre_i(c,word[cur+1])==pre_i(c,word[cur])){
				if(pre_i(c,word[i+1])!=pre_i(c,word[i])){
						//return pan(c,word,cur+1,n);
			//			cout<<i<<endl;
						f=0;
						break;
				}
			if(f) return c.end();
			else return pre_i(c,word[i+1]);
	}
}
void cals(WordOff& fv,float &var,VS vs,WordOff vf,MMap&c,string word[],int cur,int n,MMap::iterator f){
	if(cur==n){
		float tmp=cal_var_vec(vf,vs);
		if(var==-1) var=tmp;
		if(var==0)
		{
			fv=vf;
		}

		if(var>=tmp)
		{
			var=tmp;
			fv=vf;
		}
	}
	else
	{
			MMap::iterator k;//
			if(cur==n-1){
				k=c.end();
			}
			else {
				k=pre_i(c,word[cur+1]);
			}
			MMap::iterator t;
			t=pan(c,word,cur,n);
			for(f;f!=t;f++){
				vf.push_back(f->second.off);
				W wf;
				wf.count=f->second.count;
				wf.total=f->second.total;
				//vs.push_back({f->second.count,f->second.total});
				vs.push_back(wf);
				if(var!=0)//&&var>=cal_var_vec(vf,vs))
//!=0)
					cals(fv,var,vs,vf,c,word,cur+1,n,k);
				vf.pop_back();
				vs.pop_back();
			}
			/*
		}
		else{
			MMap::iterator k=c.find(word[cur]);
			for(k;k!=c.end();k++){
			vf.push_back(k->second.off);
			vs.push_back({k->second.count,k->second.total});
			if(var!=0)cals(fv,var,vs,vf,c,word,cur+1,n,c.end());
			vf.pop_back();
			vs.pop_back();
			}
		}*/
	}
}
pair<string,WF> make_pai(string&T1,WF T2){
	return pair<string,WF>(T1,T2);
}
void calave(MMap&c,string word[],int n,int k,WordOff ::iterator &l,WordMap & M){
	WordLocate::iterator v=M[word[k]].begin();
	advance(v,*l);
	WordOff::iterator f;
	//int flag=1;
	/*for(int i=0;i<k;i++){
		if(word[i]==word[k]){
			flag=0;
			break;
		}
	}*/
	for(f=v->offset.begin();f!=v->offset.end();f++){
		c.insert(make_pai(word[k],{v->count,v->total,*f}));
	}
}
void calscore(string wordi[],int ko[],int ni,ScoreLocate ::iterator &ti,WordMap & M){
	MMap ci;
	WordOff ::iterator l=ti->off.begin();
	int k=0;
	for(l;l!=ti->off.end();l++){
		calave(ci,wordi,ni,k,l,M);
		k++;
	}
	WordOff fvi;
	WordOff vfi;
	VS vsi;
	float vari=-1;
	MMap::iterator tk=pre_i(ci,wordi[0]);
	if(ni==1){
		fvi.push_back(tk->second.off);
		vari=tk->second.off;
	}
	else
		cals(fvi,vari,vsi,vfi,ci,wordi,0,ni,tk);

	/*for(MMap::iterator w1=ci.begin();w1!=ci.end();w1++)
		cout<<"word:"<<w1->first<<"	off:"<<w1->second.off<<" count:"<<w1->second.count<<" total:"<<w1->second.total<<endl;
		*/


	fvi.sort();

	/*WordOff::iterator fk=fvi.begin();
	for(fk;fk!=fvi.end();fk++)
		cout<<*fk<<endl;
		*/

	ti->poff=fvi;
	ti->score=vari;
}
void sortfile(WordMap M,string word[],int ko[],int n,ScoreLocate &tmp){
	ScoreLocate ::iterator t=tmp.begin();
	for(t;t!=tmp.end();t++){
		//cout<<"filename:"<<t->filename<<endl;
		calscore(word,ko,n,t,M);
	}
}
bool op(FS &f1,FS &f2){
	return f1.score<f2.score;
}
void sort_file_off(string w[],int ko[],int n,WordMap M,ScoreLocate &tmp,std::ostringstream &os){
	//打印分词结果
	/*for(int i=0;i<n;i++)
		cout<<w[i]<<" "<<ko[i]<<endl;
	*/

	combine(tmp,M,w,ko,n);
	if(tmp.size()==0){
		os<<"<lable>未找到</lable>";
	}
	else{
		sortfile(M,w,ko,n,tmp);
		tmp.sort(op);
	}
}
void create_info(list <FILE_S> &fileInfo){
	string name="./rule";//文件所在的目录的路径
	createfilelist(name,fileInfo);//创建保存文件信息的list
}
void create_Map(WordMap&wordmap,DICTION myMapWord,list <FILE_S> fileInfo){
	DICTION myMapChar;
	/****************** load word dictionary  **************************/
	const char *s1="charfreq.txt";
	myMapChar=readTxt(s1,myMapChar);//调用readTxt文件得到map
	//system("iconv charfreq.txt -f UTF-8 -t ASCII -o charfreq.txt");
	fileWordMap(wordmap,fileInfo,myMapWord);
}
void deal_with_res2(DLST res,DLST &wordlist){
	DLST restmp;
	DLST:: iterator p,q;
	unique_copy(res.begin(),res.end(),back_inserter(restmp));
	int k;Word wp;
	for(p=restmp.begin(),q=p,q++;q!=restmp.end();p++,q++){
		 k=(q->off-p->off)/p->keyword.length();
		wp.keyword=p->keyword;wp.off=k;
		//wordlist.push_back({p->keyword,k});
		wordlist.push_back(wp);
	}
	 k=(res.back().off-p->off)/p->keyword.length()+1;
	wp.keyword=res.back().keyword;wp.off=k;
	 //wordlist.push_back({res.back().keyword,k});
	 wordlist.push_back(wp);
}
void get_result(WordMap wordmap,DICTION myMapWord,list <FILE_S> fileInfo,string test,std::ostringstream &os){
	string *keyword;
	int *keyoff;
	STRLIST tmp;
	DLST res,rest;
	tmp=split(test);
	res=divide(test,myMapWord);
	DLST:: iterator q=res.begin();
	os<<"<lable>查询关键字</lable>:<h3>";
	for(q;q!=res.end();q++)
		os<<q->keyword<<" ";
	os<<"</h3>";
	/*DLST:: iterator p;
	for(p=res.begin();p!=res.end();p++){
		cout<<p->keyword<<" "<<p->off<<endl;
	}
	cout<<endl;*/
	DLST wordlist;
	deal_with_res2(res,wordlist);

	int n=wordlist.size();
	keyword=new string[n];
	keyoff=new int[n];
	wordlist.sort();
	trans_to_string(wordlist,keyword,keyoff);

	ScoreLocate result;
	sort_file_off(keyword,keyoff,n,wordmap,result,os);
	//打印打分结果
	/*ScoreLocate::iterator v;
	for(v=result.begin();v!=result.end();v++)
		pwrit_Va(v);
		*/
	if(result.size()==0);
	else{
		ABS_LS return_list;
		return_list=final_abs_list(result,fileInfo,keyword,n);
		ABS_LS::iterator p;
		for(p=return_list.begin();p!=return_list.end();p++)//<<"<em>""</em>"<<"
			os	<<"<h3 class=\"r\"><a href=\""<<"\">"<<(*p).filename<<"</a></h><br>"<<"<span class=\"st\">"<<(*p).abstract<<"<br>"<<"</span>";
	
	}

}

vector<string> msgs;
vector<pair<crow::response*, decltype(chrono::steady_clock::now())>> ress;

int main(int argc,char *argv[])
{
    crow::SimpleApp app;
    crow::mustache::set_base(".");

	DICTION myMapWord;
	const char *s2="wordfreq.txt";
	myMapWord=readTxt(s2,myMapWord);
	list <FILE_S> fileInfo;
	create_info(fileInfo);
	WordMap wordmap;

	/*if(atoi(argv[1]) ){
		cout<<"创建索引并写入磁盘->"<<endl;
		create_Map(wordmap,myMapWord,fileInfo);
		//pwrite_map(wordmap);
		write_data_base(wordmap);
	}
	else{
		cout<<"从磁盘读索引->"<<endl;
	}*/
	
	read_stru(wordmap);

    CROW_ROUTE(app, "/")
    ([]{
        crow::mustache::context ctx;
        return crow::mustache::load("b.html").render();
    });
	CROW_ROUTE(app,"/send")
		.methods("GET"_method,"POST"_method)([&wordmap,&myMapWord,&fileInfo](const crow::request&req){
				std::ostringstream os;
				get_result(wordmap,myMapWord,fileInfo,req.body,os);
				return crow::response{os.str()}; 	
	});

    /*CROW_ROUTE(app, "/logs")
    ([]{
        //CROW_LOG_INFO << "logs requested";
        crow::json::wvalue x;
        int start = max(0, (int)msgs.size()-100);
        for(int i = start; i < (int)msgs.size(); i++)
            x["msgs"][i-start] = msgs[i];
        x["last"] = msgs.size();
        CROW_LOG_INFO << "logs completed";
        return x;
    });
	*/

 //   CROW_ROUTE(app, "/logs/<int>")
  //  ([](const crow::request& /*req*/, crow::response& res, int after){
   /*     CROW_LOG_INFO << "logs with last " << after;
        if (after < (int)msgs.size())
        {
            crow::json::wvalue x;
            for(int i = after; i < (int)msgs.size(); i ++)
                x["msgs"][i-after] = msgs[i];
            x["last"] = msgs.size();

            res.write(crow::json::dump(x));
            res.end();
        }
        else
        {
            vector<pair<crow::response*, decltype(chrono::steady_clock::now())>> filtered;
            for(auto p : ress)
            {
                if (p.first->is_alive() && chrono::steady_clock::now() - p.second < chrono::seconds(30))
                    filtered.push_back(p);
                else
                    p.first->end();
            }
            ress.swap(filtered);
            ress.push_back({&res, chrono::steady_clock::now()});
            CROW_LOG_DEBUG << &res << " stored " << ress.size();
        }
    });

    CROW_ROUTE(app, "/send")
        .methods("GET"_method, "POST"_method)
    ([](const crow::request& req)
    {
        CROW_LOG_INFO << "msg from client: " << req.body;
        broadcast(req.body);
        return "";
    });

	*/
    app.port(4008)
        //.multithreaded()
        .run();
}

bool is_utf8_string(char *utf)  {
        int length = strlen(utf);
        int check_sub = 0;
        int i = 0;

        if ( length > CHECK_LENGTH )  //只取前面特定长度的字符来验证即可
        {
            length = CHECK_LENGTH;
        }

        for ( ; i < length; i ++ )
        {
            if ( check_sub == 0 )
            {
                if ( (utf[i] >> 7) == 0 )         //0xxx xxxx
                {
                    continue;
                }
                else if ( (utf[i] & 0xE0) == 0xC0 ) //110x xxxx
                {
                    check_sub = 1;
                }
                else if ( (utf[i] & 0xF0) == 0xE0 ) //1110 xxxx
                {
                    check_sub = 2;
                }
                else if ( (utf[i] & 0xF8) == 0xF0 ) //1111 0xxx
                {
                    check_sub = 3;
                }
                else if ( (utf[i] & 0xFC) == 0xF8 ) //1111 10xx
                {
                    check_sub = 4;
                }
                else if ( (utf[i] & 0xFE) == 0xFC ) //1111 110x
                {
                    check_sub = 5;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                if ( (utf[i] & 0xC0) != 0x80 )
                {
                    return false;
                }
                check_sub --;
            }
        }
        return true;
}
string codeTransfer(char *in_gb2312)//in_gb2312 points to the input of user;
{
	bool code;
	//char *in_gb2312="\D6л\AA\C8\CB\C3񹲺͹\FA";//test here,in_gb2312 points to the output of user;
	code=is_utf8_string(in_gb2312);
	int OUTLEN = 300;
	int INLEN=300;
	char out[300];
	char code1[10]="gb2312";
	char code2[10]="utf-8";
	string res_str;
	//char *out=new char[OUTLEN+1];
	if(code){
		//cout<<"utf_8"<<endl;//do ssomething
		res_str=string(in_gb2312);
	}
	else
	{       // gb2312-->utf-8
		CodeConverter cc2 = CodeConverter(code1,code2);
		cc2.convert(in_gb2312,INLEN,out,OUTLEN);//out is the result
		//out[OUTLEN+1]='\0';
		cout<<"gb2312->utf-8 in="<<in_gb2312<<",out"<<out<<endl;
		res_str=string(out);
	}
  return res_str;
}

DICTION readTxt(const char *s,DICTION temp)
{
	string key,line;
	fstream file;
	char *pt;
	int value;
	//cout<<"file name:"<<s<<endl;
	file.open(s,std::ios::in|std::ios::out);//打开指定文件
	if(!file){
		//cout<<"error can't open file:"<<file<<endl;
	}
	else{
		//cout<<"open file success"<<endl;
		while(!file.eof())//一直读到文件末尾
		{
			getline(file,line);//从file文件读取一行到line
			pt=strtok(&line[0],"	");//以tab键为分隔符分割读取到的一行
			int m=0;
			while(pt!=NULL)//当分割的字符串还没到尾时
			{
				m++;//用来标记是key还是value
				if(m%2!=0)
					key=pt;//第一次分割出来的是key
				else
				{
					value=atoi(pt);//把字符串转换成int整数
					//temp[key]=value;
					temp.insert(pair<string,int>(key,value));//在map中插入<key,value>值对
				//	cout<<key<<"*** "<<value<<endl;
				}
				pt=strtok(NULL,"	");//继续分割,NULL的作用只是为了使得每次调用时，都不是从头开始，而是从上次调用时查找所停止的位置开始
			}
		}
	}
	return temp;//返回建立好的map
}

DLST divide(string sentence,DICTION w_dictionary){//to splitsentence to serveral word
	STRLIST tmp;
	DLST res,rest;
	DLST:: iterator p;
	STRLIST:: iterator k;

  	tmp=split(sentence);
	/*for(k=tmp.begin();k!=tmp.end();k++){
		cout<<(*k).sentence<<"\t"<<(*k).off<<endl;
	}*/
	res=all_divi_res(tmp,w_dictionary);
	/*for(p=res.begin();p!=res.end();p++){
		cout<<(*p).keyword<<"\t"<<(*p).off<<endl;
	}*/
	return res;
}
void trans_to_string(DLST res,string * keyword,int * keyoff){// DLST type transform to string array;
	DLST:: iterator p;
	int i;
	for(i=0,p=res.begin();p!=res.end();p++)  // DLST type transform to string array;
	{
		keyword[i]=p->keyword;
		keyoff[i]=p->off;
		i++;
	}
}
STRLIST split(string str)
 {
     string Char;
		 set<string> CHAR;
     Char=read();
     judge(Char,CHAR);
     STRLIST tmp_word;
     tmp_word=split_minor(str,CHAR) ;
     return tmp_word;
 }
string read()
{
  ifstream in("punc.txt", ios::in);
 	istreambuf_iterator<char> beg(in), end;
 	string Char(beg, end);
 	in.close();
  return Char;
}
void  judge(string in_str,set<string> &CHAR)
{
     string bit1;
     string bit2;
     string bit3;
     for(int i=0;i<=in_str.size();i++)
     {

         if(in_str[i]>0)
	 {
	     //bit1.clear();
	     bit1=in_str[i];
	     //bit1+=bit1[i];
	     CHAR.insert(bit1);
	     continue;
	 }
	 else if(in_str[i]< 0)
	 {
             bit2[0]=in_str[i];
	     bit2[1]=in_str[i+1];
             bit2[2]=in_str[i+2];
	     bit3.assign(in_str,i,3);
	     i++;
	     i++;
	     CHAR.insert(bit3);

	 }
     }
}

STRLIST split_minor(string str,set<string> CHAR)
{
  int i;
  int off=0,curr=0;
  STRLIST res;
  set<string>::iterator iter;
  STR sen;
  string tmp,sentence;
  for(;curr<str.size();){
    if(str[curr]<0){
      tmp.assign(str,curr,3);
      iter=CHAR.find(tmp);
      if(iter==CHAR.end()){//not punc
		if(sentence.size()!=0){
			if(sentence[0]<0){
				sentence+=tmp;
				curr+=3;
			}
			else{
			sen.sentence=sentence;
			sen.off=off;
			res.push_back(sen);
			sentence.clear();
			sentence+=tmp;
			off=curr;
			curr+=3;
			}
		}else{
			sentence+=tmp;
			curr+=3;
		}
      }else{//punc
      if(sentence.size()!=0){
        sen.sentence=sentence;
        sen.off=off;
		res.push_back(sen);
        sentence.clear();
       // off=curr+3;
      }
      curr+=3;
	  off=curr;
      }
    }else{
        if(sentence.size()==0){
	    		if(!ispunct(str[curr]) && /*str[curr]!=32*/!isspace(str[curr])){
            tmp.assign(str,curr,1);
            sentence+=tmp;
		 				curr++;
					}else {
						 curr++;off=curr;
					 }
      	}else{
          if( (str[curr] >= 65 && str[curr] <= 90 || str[curr] >=97 && str[curr] <= 122) && (sentence[0] >= 65 && sentence[0] <= 90 || sentence[0] >=97 && sentence[0] <= 122)){// to judge letter
            tmp.assign(str,curr,1);
            sentence+=tmp;
            curr++;
          }else if((str[curr] >= 48 && str[curr] <= 57) && (sentence[0] >= 48 && sentence[0] <= 57)){//to judge number
            tmp.assign(str,curr,1);
            sentence+=tmp;
            curr++;
          }else {//标点，或是aksjh123
            sen.sentence=sentence;
            sen.off=off;
	    			res.push_back(sen);
            sentence.clear();
            if(ispunct(str[curr]) || /*str[curr]==32*/isspace(str[curr])){
		 					//if(str[curr]!=32){
		     				curr++;
				 				off=curr;
		 					/*}else{*/
		     				//str[curr]=str[curr+1];
		     				//curr++;
		 					/*}*/
	    			}
            off=curr;
            }
        }
    }
  }
	 if(sentence.size()!=0){
        sen.sentence=sentence;
        sen.off=off;
		res.push_back(sen);
        sentence.clear();
       // off=curr+3;
      }
     return res;
}


DLST one_divi_res(STR Str,DICTION w_dictionary){//to divide one string to serveral words
	DLST t;
	string str;
	string::iterator p;
	Word word,tmp;
	int p1=0,p2=2;
	str=Str.sentence;
	for(p=str.begin();p!=str.end(),p1<str.length();){
			if(p1+3*p2>str.length())
				{
					p1+=3;
					p2=2;
					continue;
				}
			tmp.keyword.assign(str,p1,3*p2);
			//cout<<p1<<" "<<p2<<" "<<tmp.keyword<<endl;
			if(w_dictionary.count(tmp.keyword)==0)//when there is no word can be found in dictionary
			{
				p2++;
				continue;
			}else {//find word
				word.keyword=tmp.keyword;
				word.off=p1+Str.off;
				t.push_back(word);
				//cout<<word.keyword<<p2<<endl;
				p2++;
			}
	}
	return t;
}
DLST all_divi_res(STRLIST str,DICTION w_dictionary){ //to find all the words which  starts with the character appearing in the search statement,and then deal with them
	string tmp;
	STR search_sentence;
	DLST all,sub,subres,result;
	STRLIST:: iterator slist;
	DLST::iterator dl;
	Word num_letter;
	for(slist=str.begin();slist!=str.end();slist++){
		search_sentence=(*slist);
		if(search_sentence.sentence[0]<0){
			sub=one_divi_res(search_sentence,w_dictionary);
			subres=last_choose_res(sub,search_sentence.sentence);
			result=deal_with_res(subres,(*slist));
			dl=all.end();
			//cout<<result.size();
			all.splice(dl,result);
		}else{
			num_letter.keyword=search_sentence.sentence;
			num_letter.off=search_sentence.off;
			all.push_back(num_letter);
		}

	}
	return all;

}

//Adopt  oen strategy to decide the final consequence of the combination of key words
DLST last_choose_res(DLST aL,string search){
	DLST res;
	DLST amb_r;//ambiguity range
	Word tmp;
	DLST:: iterator p,t,p_flag;
	int i=0,offset;
	//cout<<aL.size()<<endl;
	if(aL.size()!=0){
		tmp=aL.front();
		offset=tmp.off;
		res.push_back(tmp);
		for(p=aL.begin();p!=aL.end();p++)
		{
			if(tmp.off==(*p).off){
				if((tmp.keyword.find((*p).keyword))==string::npos){
					tmp=(*p);
					res.pop_back();
					res.push_back(tmp);
					offset=tmp.off+tmp.keyword.length();
				}
			}else{
				if(offset<=(*p).off){
					res.push_back((*p));
					tmp=(*p);
					offset=tmp.off+tmp.keyword.length();
				}
			}
		}
	}
	return res;
}

DLST deal_with_res(DLST res,STR str){  // to get the splitting result of one substring
	int length,offset;
	Word tmp;
	DLST:: iterator p;
	DLST  sp;
	length=res.size();
	int *arr=new int[length];
	//tmp=res.front();
	offset=str.off;
	if(length!=0){
		for(p=res.begin();p!=res.end();p++){
			if(offset<(*p).off){
					for(int i=offset;i+str.off<(*p).off;){
							tmp.keyword.assign(str.sentence,i,3);
							tmp.off=offset;
							sp.push_back(tmp);
							offset+=3;
							i+=3;
					}
			}
			sp.push_back((*p));
			offset=(*p).off+(*p).keyword.length();
		}
	}else{
		for(int i=0;i<str.sentence.length();){
				tmp.keyword.assign(str.sentence,i,3);
				tmp.off=offset;
				sp.push_back(tmp);
				offset+=3;
				i+=3;
		}
	}
	return sp;
}
int createfilelist(string path,list <FILE_S> &fileInfo)
{
    DIR *d;
    struct dirent *file;
    struct stat buf;
    FILE_S fl;
    char *temp=NULL,*tempname=NULL,*pt=NULL,*p=NULL; //分割文件名用到的中间变量
    int mark,index;//分割文件名用到的中间变量
    ifstream infile;
    if(!(d = opendir(path.c_str())))//打开目录
    {
        cout<<"error opendir !!!"<<endl;
        return -1;
    }
	char cwd[256];
	string tempstr;
	getcwd(cwd,256);
    chdir(path.c_str());//Add this, so that it can scan the children dir(please look at main() function),change current directory改变当前目录，进入目录
    while((file = readdir(d)) != NULL)//读该目录下所有的文件
    {

        if(strcmp(file->d_name, ".") == 0)//过滤掉.和..文件
            continue;
        else if(strcmp(file->d_name,"..")==0)
            continue;
        if(stat(file->d_name, &buf) >= 0 && !S_ISDIR(buf.st_mode) )//stat()通过文件名获取文件信息，并保存在buf所指的结构体stat中
        {
           fl.name=string(file->d_name);//file->d_name是char数组,
           temp=file->d_name;
           //分词，以~为分割符,分割文件名,取出文件编号
           pt=strtok(temp,"~"); //string 转char*
	         mark=0;
	         index=0;
           while(pt!=NULL)
        	   {
      		   mark++;
      		   if(mark==1)
      		   {
      		   	fl.flag=atoi(pt); //fileno
      		   	//cout<<"file-no: "<<fl.flag<<endl;
      		   }
      		   pt=strtok(NULL,".");//.为分隔符,分割文件名,取出文章的题目
      		   index++;
      		   if(index==1)
      		   {
      			fl.title=pt;
      			//cout<<"file-title: "<<fl.title<<endl;
      		   }

      	   }
					 fl.article="";
           //.为分隔符,分割文件名,取出文章的题目
	         infile.open(fl.name.c_str());//打开文件,读取文件内容，将文件内容写入FILE_S结构体
           while(!infile.eof()){
						 getline(infile,tempstr);
						 fl.article=fl.article+tempstr;
           }
           //cout<<"file-content: "<<fl.article<<endl;
           infile.close();//fstream的close()函数直接关闭文件
           fileInfo.push_front(fl);//将文件结构体放到文件信息的list中
        }
    }
    //fileInfo.push_back(fl);
    closedir(d);
	chdir(cwd);
    return 0;
}

void fileWordMap(WordMap &wordmap,list <FILE_S> fileInfo,DICTION & w_dictionary)
{
	int fileCount;//文章总数
	int totalWord;
	WordLocate Filelist;
	Word word;
	//string SearchWord;
	//WordMap wordmap;
	WordMap::iterator iter;
	list <FILE_S>::iterator fit;
	DLST Dlst;
	for(fit=fileInfo.begin();fit!=fileInfo.end();fit++)
	{
		Dlst=divide((*fit).article,w_dictionary);//将文章进行分词，返回一个list
		totalWord=Dlst.size();
		for(DLST::iterator it=Dlst.begin();it!=Dlst.end();it++)//文章的每一个词都在map里进行查找
		{
			word=*it;//取出这个word

			//iter=wordmap.find((*it).keyword);
			iter=wordmap.find(word.keyword);//找这个词在不在map里面,find函数找到的话就是返回那个word所在的iterator，找不到就返回map.end()
			if(iter==wordmap.end())	//找不到这个word
			{
				addTomap(wordmap,word,(*fit),totalWord);//添加到map
			}
			else//找到了这个word
			{
				Filelist=iter->second;//返回的是value，也就是整个list
				updateMap(wordmap,iter,Filelist,word,(*fit),totalWord);//更新map
			}
	  	}

	}
	/*
	cout<<Dlst.size()<<endl;
	//输出map
	WordMap::iterator it2;//遍历wordmap,输出wordmap
	WordLocate::iterator k;
	WordOff::iterator m;
	cout<<wordmap.size()<<endl;
	for(it2=wordmap.begin();it2!=wordmap.end();++it2)
	{
		cout<<"key:"<<it2->first;
		Filelist=it2->second;
		for(k=Filelist.begin();k!=Filelist.end();k++)
		{
			cout<<"  the  value:"<<"filename: "<<(*k).filename<<"word count:"<<(*k).count<<"totalWord:"<<(*k).total;
			cout<<" offset:";
			for(m=(*k).offset.begin();m!=(*k).offset.end();m++)
				cout<<*m<<" ";
			cout<<endl;
		}
	}
	*/
}

void addTomap(WordMap &wordmap,Word word,FILE_S file,int totalWord)
{
	Va InsertFile;
	WordLocate Filelist;
	InsertFile.filename=file.name;//要插入到filelist的结构体
	InsertFile.count=1;
	InsertFile.total=totalWord;
	InsertFile.offset.push_back(word.off);

	Filelist.push_back(InsertFile);
	wordmap.insert(pair<string,list<Va> >(word.keyword,Filelist));//建立map
}

void updateMap(WordMap &wordmap,WordMap::iterator it,WordLocate Filelist,Word word,FILE_S file,int totalWord)
{
	WordLocate::iterator ite;
	Va InsertFile;
	int flag=0;
	for(ite=Filelist.begin();ite!=Filelist.end();ite++)
	{
		if((*ite).filename==file.name)//这个文件在list里面
		{
			(*ite).count=(*ite).count+1;//出现的次数++
			(*ite).offset.push_back(word.off);//把词的偏移量放进list
			break;
		}
		else
		{
			flag++;
		}
	}
	//这个文件不在list里面
	if(flag==Filelist.size())
	{
		InsertFile.filename=file.name;
		InsertFile.count=1;
		InsertFile.total=totalWord;
		InsertFile.offset.push_back(word.off);
		Filelist.push_back(InsertFile);
	}
	wordmap.erase(it);//删除这个<key,value>键值对，重新添加更改后的<key,value>
	wordmap.insert(pair<string,list<Va> >(word.keyword,Filelist));
}

string STRCAT(string a){
	string b="<em>";
	string c=b+a+"</em>";
	return c;
}

string mode_matct(string in_str,string key)
{
	string::size_type position;
	string str=in_str;
	string F=key;//匹配模式
	string temp_str=STRCAT(F);
	int num = 0;
	size_t fi = str.find(F, 0);
    	while (fi!=str.npos)
    	{
        	str=str.replace(fi,F.size(),temp_str);
        	num++;
        	fi = str.find(F, fi + 5);
    	}
    	return str;
}

ABS get_file_abs(FS file,list <FILE_S> fileInfo,string * keyword,int n){ //to get the abstract of one file
	list <FILE_S>:: iterator p;
	WordOff off_l;
	//WordOff::iterator o,t;
	WordOff::iterator cur,next;
	int start,num,size=0,tmpoff;
	string content,subabs,tmpstr;
	ABS abs_str;
	abs_str.filename=file.filename;
	for(p= fileInfo.begin();p!= fileInfo.end();p++){
		if(file.filename==(*p).name){
			content=(*p).article;
			break;
		}
	}
	size=file.poff.size();

	tmpoff=0;
	off_l=file.poff;
	for(cur=off_l.begin();cur!=off_l.end();cur++){
			next=cur;
			next++;
			//subabs.assign(content,cur->offword,cur->lenword);
			//subabs="<em>"+subabs+"</em>";
			start=*cur;
			if((*cur+180)!=content.size()){
				if(next!=off_l.end()){
					if(start+180>=*next)
						num=(*next)-start;
					else{
						num=180;
						while(1){
							if((content[start+num]  & 0xF0)==0xE0)
								break;
							else num--;
						}
					}
				}else{
					num=180;
					while(1){
						if((content[start+num]  & 0xF0)==0xE0)
							break;
						else num--;
					}
				}
			}else{
				if(next!=off_l.end()){
						num=*next-start;
					}
					else{
						num=content.size()-start;
					}
			}
			tmpstr.assign(content,start,num);
			subabs+=tmpstr;
			abs_str.abstract+=subabs;
	}

	for(int i=0;i<n;i++){
		abs_str.abstract=mode_matct(abs_str.abstract,keyword[i]);
	}
	abs_str.abstract+="......";
	return abs_str;
}

ABS_LS final_abs_list(ScoreLocate l ,list <FILE_S> fileInfo,string*keyword,int n){//to get  abstract  of each element whic is  in the list
	ScoreLocate::iterator p;
	ABS elem;
	ABS_LS res;
	for(p=l.begin();p!=l.end();p++){
		elem=get_file_abs(*p,fileInfo,keyword,n);
		res.push_back(elem);
	}
	return res;
}
