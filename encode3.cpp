#include <cv.h>
#include <highgui.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <climits>
#include <iostream>
#include <algorithm>
#include <map>

using namespace cv;

typedef long long ll;


const string table = ".#:;0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const int MAX_COLOR = 17000000;
const int COLOR_KIND_ADJ = 70;//色の種類を制限するための調整値
const int GOOD_ROW = 150;//適した行数
const int GOOD_COL = 400;//適した列数

const double FPS = 0.03;//PCのFPS


const string HTML_DATA_1 = "\
<html>\n\
<head>\n\
  <meta http-equiv=\"Content-Type\" \n \
        content=\"text/html; charset=iso-2022-jp\"> \n \
<title>CharacterMovie</title> \n \
<script type = \"text/javascript\" src = \"player.js\"> \n \
</script> \n";

const string HTML_DATA_2 = "\
</head> \n \
<style> \n \
BODY{ \n \
    background-color:  #FFFFFF; \n \
    color:             #000000; \n \
    overflow:          hidden; \n \
} \n \
PRE{ \n \
    font-size:         5px; \n \
    font-family:       ＭＳ ゴシック,Osaka-等幅,monospace; \n \
    line-height:       5px; \n \
    letter-spacing:    0px; \n \
} \n \
</style> \n \
<body> \n \
<input type=\"button\" onClick=\"Play()\" value=\" Play \">&nbsp; \n \
<span id=\"info_load\" style=\"width:25px; text-align:right\"></span>&nbsp;&nbsp;&nbsp;&nbsp; \n \
<DIV id=\"canvas\"></DIV> \n \
</body> \n \
</html> \n";


int Count[MAX_COLOR];//色の出現回数
char ID[MAX_COLOR];//色->文字へのテーブル
std::vector<std::pair<ll,int> >ids;//色情報pair<回数,id>
std::vector<std::vector<char> >matrix;//色の行列
string DIR;
Mat rgb;//画像ピクセル行列
//画像の大きさ
int ROW;//行
int COL;//列



void init();//初期化
void init_picture(Mat matrix);//画像の初期化
void init_picture(string PATH);//画像の初期化
void SortColor();//色を使用頻度順に並び替える
int rgbToid(int r,int g,int b);//RGB -> IDへの変換
Mat adjustSize(IplImage* ipl);//丁度良い大きさに変換する
void WriteToFile(string PATH,ll num);//ファイルにデータを書き込む

void PictureMake(Mat matrix,ll num);//画像の色行列から文字画像を作成する
void MovieMake(string PATH);//動画から1フレームごとに画像を切り出し、文字化
void HTMLMake(string PATH,ll all_num);//HTMLファイルを作成する

void CalcMovieColor(string PATH);//動画の色の統計をとる
void CalcColorStatus(Mat pic_matrix,ll num);//画像の色の統計をとる

string LLToString(ll i);//long long |-> string 変換

int main( int argc, char** argv )
{
  if(argc != 3){
    printf("usage: encode [moviepath] [writepath]\n");
  }
  else{
    string movie_path = argv[1];
    DIR = argv[2];
    MovieMake(movie_path);
  }
  return 0;
}


int rgbToid(int r,int g,int b){//RGB -> IDへの変換
  //色の種類を調整する--------
  r = floor(r / COLOR_KIND_ADJ) * COLOR_KIND_ADJ;
  g = floor(g / COLOR_KIND_ADJ) * COLOR_KIND_ADJ;
  b = floor(b / COLOR_KIND_ADJ) * COLOR_KIND_ADJ;
  //-----------------------
  
  ll id = 255 * 255 * std::max(r - 1,0) + 255 * std::max(g - 1,0) + b;
  return id;
}

void MovieMake(string PATH){//動画から1フレームごとに画像を切り出し、文字化
  Mat img;
  VideoCapture cap;
  if(cap.open(PATH)){//動画読み込み
    img = cap.open(PATH);
    printf("load movie complete\n");
  }
  else{
    printf("Cannot load movie\n");
    return;
  }
  
  init();//初期化
  CalcMovieColor(PATH);//動画の統計をとる
  
  ll num = 0;
  // 画像の1フレーム出力
  for(num = 0;;num++){
      // 出力ストリームで画像の抽出が可能
      cap >> img;
      // imgの中に画像の情報がなければ、動画の終了を意味する      
      if(img.empty()) break;
      PictureMake(img,num);//文字画像の作成
    }
  
  
  HTMLMake(DIR.substr(0,DIR.size() - 1) + ".html",num);
  printf("Finish......\n");
}

void PictureMake(Mat pic_matrix,ll num){//画像の色行列から文字画像を作成する
  string filename = LLToString(num) + ".js";
  string output_path = DIR + filename;
  
  init_picture(pic_matrix);//画像の初期化
  
  WriteToFile(output_path,num);//文字画像を出力
}

void CalcMovieColor(string PATH){//動画の色の統計をとる
  Mat img;
  VideoCapture cap;
  if(cap.open(PATH)){//動画読み込み
    img = cap.open(PATH);
    printf("load movie complete\n");
  }
  else{
    printf("Cannot load movie\n");
    return;
  }

  ll num = 0;
  // 画像の1フレーム出力
  for(num = 0;;num++){
    // 出力ストリームで画像の抽出が可能
    cap >> img;
    // imgの中に画像の情報がなければ、動画の終了を意味する      
    if(img.empty()) break;
    CalcColorStatus(img,num);
  }
  SortColor();
  printf("Calc Complete\n");
}

void CalcColorStatus(Mat pic_matrix,ll num){
  init_picture(pic_matrix);//画像の初期化
  for(int y = 0; y < ROW; y++){
    for(int x = 0; x < COL; x++){
      Vec3b bgr = rgb.at<Vec3b>(y,x);
      int id = rgbToid(bgr[0],bgr[1],bgr[2]);//(R,G,B) |-> (ID)へ変換
      Count[id]++;
    }
  }
}


void HTMLMake(string PATH,ll all_num){//HTMLファイルを作成する
  FILE *outputfile = fopen(PATH.c_str(),"w");
  string datas = "";
  for(ll i = 0; i < all_num; i++){
    char buf[100]; sprintf(buf,"<script type = \"text/javascript\" src = \"%s%lld.js\"></script>\n",DIR.c_str(),i);
    string tmp = buf;
    datas += tmp;
  }
  string HTML_DATA = HTML_DATA_1 + datas + HTML_DATA_2;
  fprintf(outputfile,"%s",HTML_DATA.c_str());
  fclose(outputfile);
}
//初期化
void init(){
  memset(ID,'0',sizeof(ID));
  memset(Count,0,sizeof(Count));
  ids.clear();
  for(int i = 0; i < MAX_COLOR; i++) ID[i] = table[i % table.size()];
}

//画像の初期化
void init_picture(Mat rgb_mat){
  IplImage tmp = rgb_mat;
  IplImage* tmp_Image = &tmp;
  rgb = adjustSize(tmp_Image);
  ROW = rgb.rows;//行
  COL = rgb.cols;//列
}

//画像の初期化
void init_picture(string PATH){  
  IplImage* tmp_Image = cvLoadImage (PATH.c_str(), CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
  rgb = adjustSize(tmp_Image);
  printf("%d x %d\n",rgb.rows,rgb.cols);
  ROW = rgb.rows;//行
  COL = rgb.cols;//列
}


//色を使用頻度順に並び替える
void SortColor(){
  for(int id = 0; id < MAX_COLOR; id++){
    if(Count[id] > 0) ids.push_back(std::make_pair(Count[id],id));
  }  
  sort(ids.rbegin(),ids.rend());//色の出現頻度順でソート

  //出現数が多い物程分かりやすい(?)文字を与える
  for(int i = 0; i < std::min((int)table.size(),(int)ids.size()); i++){
    int id = ids[i].second;
    int NUM = ids[i].first;
    ID[id] = table[i];
  }
}

//ファイルにデータを書き込む
void WriteToFile(string PATH,ll num){
  FILE *outputfile = fopen(PATH.c_str(),"w");
  string dat = "dat[" + LLToString(num) + "] = \"";
  fprintf(outputfile,"%s",dat.c_str()); 
  for(int y = 0; y < ROW; y++){
    for(int x = 0; x < COL; x++){
      Vec3b bgr = rgb.at<Vec3b>(y,x);
      int id = rgbToid(bgr[0],bgr[1],bgr[2]);//(R,G,B) |-> (ID)へ変換
      fprintf(outputfile,"%c",ID[id]);
    }
    
    fprintf(outputfile,"<br>\\\n");
  }
  fprintf(outputfile,"\";");
  fclose(outputfile);
}
Mat adjustSize(IplImage* ipl){//丁度良い大きさに変換する
  //適切な大きさに拡大 or 縮小
  double py = (double)GOOD_ROW / ipl->height;
  double px = (double)GOOD_COL / ipl->width;

  IplImage* resize;
  CvSize size = cvGetSize(ipl);
	
  resize = cvCreateImage(cvSize(size.width * px, size.height * py), IPL_DEPTH_8U, 3);
  cvResize(ipl, resize);	// resize
  return cvarrToMat(resize);
}

string LLToString(ll i){//long long |-> string 変換
  char buf[100];
  sprintf(buf,"%lld",i);
  string res = buf;
  return res;
}
