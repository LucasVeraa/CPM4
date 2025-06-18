
#include "CPM.h"
#include "OpticFlowIO.h"

#include <dirent.h>   // Para listar archivos del directorio
#include <cstring>    // Para strcmp y strstr
#include <fstream>    // Para escritura de resultados en CSV

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
    if (argc < 3) {
        printf("USAGE: CPM image_folder output.csv\n");
        return -1;
    }

    string folderPath = argv[1];
    string outputCSV = argv[2];

    vector<string> imageFiles;

    // Leer nombres de archivos en el directorio
    DIR* dir = opendir(folderPath.c_str());
    if (!dir) {
        perror("Error abriendo directorio");
        return -1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        string name = entry->d_name;
        if (name.length() > 4 && (strstr(name.c_str(), ".png") || strstr(name.c_str(), ".jpg") || strstr(name.c_str(), ".bmp"))) {
            imageFiles.push_back(folderPath + "/" + name);
        }
    }
    closedir(dir);

    // Abrir archivo de salida CSV
    ofstream outFile(outputCSV);
    outFile << "imagen1,imagen2,resultado\n";

    // Comparar cada imagen con todas las demás
    for (size_t i = 0; i < imageFiles.size(); ++i) {
        for (size_t j = 0; j < imageFiles.size(); ++j) {

            FImage img1, img2;
            img1.imread(imageFiles[i].c_str());
            img2.imread(imageFiles[j].c_str());

            if (img1.width() != img2.width() || img1.height() != img2.height()) {
                printf("Saltando comparación entre %s y %s (dimensiones diferentes)\n", imageFiles[i].c_str(), imageFiles[j].c_str());
                continue;
            }

            int w = img1.width();
            int h = img1.height();
            FImage matches;
            CPM cpm;
            cpm.SetStep(3);  // o ajusta si quieres
            cpm.Matching(img1, img2, matches);

            FImage u, v;
            Match2Flow(matches, u, v, w, h);

            int conta1=0,conta2=0;
            double menor=UNKNOWN_FLOW, mayor=-UNKNOWN_FLOW;

            for (int k = 0; k < w*h; ++k){
                if (u.pData[k] != UNKNOWN_FLOW){
                    menor = __min(menor, u.pData[k]);
                    mayor = __max(mayor, u.pData[k]);
                    conta1++;
                }
                if (v.pData[k] != UNKNOWN_FLOW){
                    menor = __min(menor, v.pData[k]);
                    mayor = __max(mayor, v.pData[k]);
                    conta2++;
                }
            }

            int tam = int(fabs(menor)) + int(fabs(mayor)) + 1;
            vector<int> hx(tam, 0), hy(tam, 0);

            for (int k = 0; k < w*h; ++k) {
                if (u.pData[k] != UNKNOWN_FLOW)
                    hx[int(u.pData[k]) + abs(int(menor))]++;
                if (v.pData[k] != UNKNOWN_FLOW)
                    hy[int(v.pData[k]) + abs(int(menor))]++;
            }

            double f = 0;
            double abajo = double(2 * conta1);
            for (int b = 0; b < tam; ++b){
                double arriba = double(hx[b] + hy[b]);
                double resultado = (arriba / abajo);
                f += resultado * resultado;
            }

            if (f != f) f = 0.0; // comprobar NaN

            // Guardar resultado en CSV
            outFile << imageFiles[i].substr(folderPath.length()+1) << ","
                    << imageFiles[j].substr(folderPath.length()+1) << ","
                    << f << "\n";

            fflush(stdout);
        }
    }

    outFile.close();
    return 0;
}
