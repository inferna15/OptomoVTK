#pragma once

#ifdef _WIN32
    #ifdef MODEL_EXPORTS
        #define MODEL_API __declspec(dllexport)
    #else
        #define MODEL_API __declspec(dllimport)
    #endif
#else
    #define MODEL_API __attribute__((visibility("default")))
#endif

#include <iostream>
#include <vtkAutoInit.h>
#include <vtkSmartPointer.h>
#include <vtkAlgorithmOutput.h>
#include <vtkDICOMImageReader.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkImageReslice.h>
#include <vtkImageMapToColors.h>
#include <vtkImageMapper.h>
#include <vtkImageData.h>
#include <vtkActor2D.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolumeProperty.h>
#include <vtkVolume.h>
#include <vtkCallbackCommand.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkProperty2D.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

class Model {
// VTK sınıfları
private:
	vtkSmartPointer<vtkDICOMImageReader> reader;
	vtkSmartPointer<vtkColorTransferFunction> colorFunc;
	vtkSmartPointer<vtkPiecewiseFunction> opacityFunc;
	vtkSmartPointer<vtkRenderWindowInteractor> interactor;
	// Axial
	vtkSmartPointer<vtkImageReslice> resliceAxial;
	vtkSmartPointer<vtkImageMapToColors> colorAxial;
	vtkSmartPointer<vtkImageMapper> mapperAxial;
	vtkSmartPointer<vtkActor2D> actorAxial;
	vtkSmartPointer<vtkRenderer> rendererAxial;
	// Sagittal
	vtkSmartPointer<vtkImageReslice> resliceSagittal;
	vtkSmartPointer<vtkImageMapToColors> colorSagittal;
	vtkSmartPointer<vtkImageMapper> mapperSagittal;
	vtkSmartPointer<vtkActor2D> actorSagittal;
	vtkSmartPointer<vtkRenderer> rendererSagittal;
	// Frontal
	vtkSmartPointer<vtkImageReslice> resliceFrontal;
	vtkSmartPointer<vtkImageMapToColors> colorFrontal;
	vtkSmartPointer<vtkImageMapper> mapperFrontal;
	vtkSmartPointer<vtkActor2D> actorFrontal;
	vtkSmartPointer<vtkRenderer> rendererFrontal;
	// Volume
	vtkSmartPointer<vtkSmartVolumeMapper> mapperVolume;
	vtkSmartPointer<vtkVolumeProperty> propertyVolume;
	vtkSmartPointer<vtkVolume> actorVolume;
	vtkSmartPointer<vtkRenderer> rendererVolume;
	// Lines
	vtkSmartPointer<vtkLineSource> lines2D[6];
	vtkSmartPointer<vtkLineSource> lines3D[15];

// Fonksiyonlar
public:
	Model();
	vtkAlgorithmOutput* readDicom(const char* path);
	void initAxial(vtkAlgorithmOutput* input);
	void initSagittal(vtkAlgorithmOutput* input);
	void initFrontal(vtkAlgorithmOutput* input);
	void initVolume(vtkAlgorithmOutput* input);
	void setRenderWindow();
	void init2DLines();
	void init3DLines();
	void XLayer(int isUp);
	void YLayer(int isUp);
	void ZLayer(int isUp);
	void XLayerMotion(int isUp);
	void YLayerMotion(int isUp);
	void ZLayerMotion(int isUp);
	void SetPanel();
	void Maximize(int panel);
	void start();
	void ChangeLayer(int panel, int dir);
	void ChangeMotion(int panel, int dir);
	void ChangeZoom(int panel, double value);
	void Reset();

// Değişkenler
private:
	int extent[6];
	double spacing[3];
	double origin[3];
	int layer[3];
	double window;
	double level;
	double ratio[3];
	int motions[3];
	double zooms[3];
	double exts[3];
	double offsets[3];
	int width;
	int height;
	double linePos2D[6][2][2];
	double linePos3D[15][2][3];

public:
	bool isMax;
	vtkSmartPointer<vtkRenderWindow> renderWindow;
};

extern "C" {
	MODEL_API Model* Start(const char* path);
	MODEL_API void Finish(Model* model);
	MODEL_API void ChangeLayer(Model* model, int panel, int dir);
	MODEL_API void ChangeMotion(Model* model, int panel, int dir);
	MODEL_API void ChangeZoom(Model* model, int panel, double value);
	MODEL_API void Reset(Model* model);
	MODEL_API void Maximize(Model* model, int panel, int isMax);
	MODEL_API vtkRenderWindow* GetWindow(Model* model);
}