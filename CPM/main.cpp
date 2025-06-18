
#include "CPM.h"
#include "OpticFlowIO.h"

//para el calculo de tiempos
#include <sys/resource.h>
#include <time.h>
#include <sys/time.h>
#include "funciones.h"

using namespace std;


// draw each match as a 3x3 color block
void Match2Flow(FImage& inMat, FImage& ou, FImage& ov, int w, int h)
{
	if (!ou.matchDimension(w, h, 1)){
		ou.allocate(w, h, 1);
	}
	if (!ov.matchDimension(w, h, 1)){
		ov.allocate(w, h, 1);
	}
	ou.setValue(UNKNOWN_FLOW);
	ov.setValue(UNKNOWN_FLOW);
	int cnt = inMat.height();

	for (int i = 0; i < cnt; i++){
		float* p = inMat.rowPtr(i);
		float x = p[0];
		float y = p[1];
		float u = p[2] - p[0];
		float v = p[3] - p[1];
		for (int di = -1; di <= 1; di++){
			for (int dj = -1; dj <= 1; dj++){
				int tx = ImageProcessing::EnforceRange(x + dj, w);
				int ty = ImageProcessing::EnforceRange(y + di, h);
				ou[ty*w + tx] = u;
				ov[ty*w + tx] = v;
			}
		}
	}
}

void WriteMatches(const char *filename, FImage& inMat)
{

	int len = inMat.height();
	FILE *fid = fopen(filename, "w");

	for (int i = 0; i < len; i++){
		float x1 = inMat[4 * i + 0];
		float y1 = inMat[4 * i + 1];
		float x2 = inMat[4 * i + 2];
		float y2 = inMat[4 * i + 3];
		fprintf(fid, "%.0f %.0f %.0f %.0f\n", x1, y1, x2, y2);
		//fprintf(fid, "%.3f %.3f %.3f %.3f 1 100\n", x1, y1, x2, y2);
	}
	fclose(fid);
}

int main(int argc, char** argv)
{

	if (argc < 3){
		printf("USAGE: CPM image1 image2 outMatchText <step>\n");
		return -1;
	}

	FImage img1, img2;

	

	img1.imread(argv[1]);
	img2.imread(argv[2]);
	char* outMatName = argv[3];
	int step = 3;
	if (argc >= 5){
		step = atoi(argv[4]);
	}

	int w = img1.width();
	int h = img1.height();
	if (img2.width() != w || img2.height() != h){
		printf("CPM can only handle images with the same dimension!\n");
		return -1;
	}

	CTimer totalT;
	FImage matches;

	CPM cpm;
	cpm.SetStep(step);
	cpm.Matching(img1, img2, matches);

	FImage u, v;
	Match2Flow(matches, u, v, w, h);

	int conta1=0,conta2=0;
	double menor=UNKNOWN_FLOW, mayor=-UNKNOWN_FLOW;

	for (int i = 0; i < w*h; ++i){
		if (u.pData[i]!=UNKNOWN_FLOW){
			fflush(stdout);
			menor=__min(menor, u.pData[i]);
			mayor=__max(mayor, u.pData[i]);
			conta1++;
		}
		if (v.pData[i]!=UNKNOWN_FLOW){
			fflush(stdout);
			menor=__min(menor, v.pData[i]);
			mayor=__max(mayor, v.pData[i]);
			conta2++;
		}
	}

	fflush(stdout);
	int tam=0;
	tam=int(fabs(menor))+int(fabs(mayor))+1;	

	int hx[tam], hy[tam];

	for (int b = 0; b < tam; ++b){
		hx[b]=0;
		hy[b]=0;
	}

	int cont=0;
	for(int b=0;b<tam;b++){
		for (int m = 0; m < w*h; ++m){
			if (int(u.pData[m])+abs(menor)==b){
				cont++;
			}
		}
		hx[b]=cont;
		cont=0;
	}

	for(int b=0;b<tam;b++){
		for (int m = 0; m < w*h; ++m){
			if (int(v.pData[m])+abs(menor)==b)
			{
				cont++;
			}
		}
		hy[b]=cont;
		cont=0;
	}

	int suma1=0;
	int suma2=0;
	for (int b = 0; b < tam; ++b){
		suma1+=hx[b];
		suma2+=hy[b];
	}

	double f=0;
	double abajo = double(2*conta1);
	for (int b = 0; b < tam; ++b){
		double arriba =double( hx[b] + hy[b]);
		double resultado = double((arriba/abajo));
		f+=resultado*resultado;
	}

	f==f? cout << f << endl: 
          cout << "0" << endl;
	//printf("%lf\n",f );




	return 0;
}
