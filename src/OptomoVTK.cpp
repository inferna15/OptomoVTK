#include "OptomoVTK.h"

extern "C" MODEL_API Model* Start(const char* path) {
	Model* model = new Model();

	vtkAlgorithmOutput* output = model->readDicom(path);
	model->initAxial(output);
	model->initSagittal(output);
	model->initFrontal(output);
	model->initVolume(output);
	model->setRenderWindow();
	model->init2DLines();
	model->init3DLines();
	model->start();

	return model;
}

extern "C" MODEL_API void Finish(Model* model) {
	if (model) {
		delete model;
	}
}

extern "C" MODEL_API void ChangeLayer(Model* model, int panel, int dir) {
	if (model) {
		model->ChangeLayer(panel, dir);
	}
}

extern "C" MODEL_API void ChangeMotion(Model* model, int panel, int dir) {
	if (model) {
		model->ChangeMotion(panel, dir);
	}
}

extern "C" MODEL_API void ChangeZoom(Model* model, int panel, double value) {
	if (model) {
		model->ChangeZoom(panel, value);
	}
}

extern "C" MODEL_API void Reset(Model* model) {
	if (model) {
		model->Reset();
	}
}

extern "C" MODEL_API void Maximize(Model* model, int panel, int isMax) {
	if (model) {
		if (isMax == 1) {
			model->isMax = true;
		} else if (isMax == 0) {
			model->isMax = false;
		}
		model->Maximize(panel);
		model->SetPanel();
	}
} 

extern "C" MODEL_API vtkRenderWindow* GetWindow(Model* model) {
	if (model) {
		return model->renderWindow;
	} else {
		return nullptr;
	}
}

#pragma region Fonksiyonlar
Model::Model() {
	renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
	renderWindow->SetSize(600, 800);
	width = 600;
	height = 800;

	auto resizeCallback = vtkSmartPointer<vtkCallbackCommand>::New();
	resizeCallback->SetCallback([](vtkObject* caller, long unsigned int eventId, void* clientData, void* callData) {
		Model* self = static_cast<Model*>(clientData);
		self->SetPanel();
	});

	resizeCallback->SetClientData(this);

	renderWindow->AddObserver(vtkCommand::WindowResizeEvent, resizeCallback);

	interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	interactor->SetRenderWindow(renderWindow);

	zooms[0] = 1;
	zooms[1] = 1;
	zooms[2] = 1;
	motions[0] = 0;
	motions[1] = 0;
	motions[2] = 0;
	isMax = false;
}

void Model::start() {
	interactor->Initialize();
	renderWindow->Render();
	interactor->Start();
}

vtkAlgorithmOutput* Model::readDicom(const char* path) {
	reader = vtkSmartPointer<vtkDICOMImageReader>::New();
	reader->SetDirectoryName(path);
	reader->Update();

	reader->GetDataExtent(extent);
	reader->GetDataSpacing(spacing);
	reader->GetDataOrigin(origin);

	double range[2];
	reader->GetOutput()->GetScalarRange(range);
	window = range[1] - range[0];
	level = window / 2;

	colorFunc = vtkSmartPointer<vtkColorTransferFunction>::New();
	colorFunc->AddRGBPoint(range[0], 0, 0, 0);
	colorFunc->AddRGBPoint((range[1] + range[0]) / 2, 0.5, 0.5, 0.5);
	colorFunc->AddRGBPoint(range[1], 1, 1, 1);

	opacityFunc = vtkSmartPointer<vtkPiecewiseFunction>::New();
	opacityFunc->AddPoint(range[0], 0);
	opacityFunc->AddPoint((range[1] + range[0]) / 2, 0.5);
	opacityFunc->AddPoint(range[1], 1);

	layer[0] = (extent[1] - extent[0]) / 2;
	layer[1] = (extent[3] - extent[2]) / 2;
	layer[2] = (extent[5] - extent[4]) / 2;

	return reader->GetOutputPort();
}

void Model::initAxial(vtkAlgorithmOutput* input) {
	resliceAxial = vtkSmartPointer<vtkImageReslice>::New();
	resliceAxial->SetInputConnection(input);
	resliceAxial->SetOutputDimensionality(2);
	resliceAxial->SetResliceAxesDirectionCosines(1, 0, 0, 0, 0, 1, 0, 1, 0);
	resliceAxial->SetResliceAxesOrigin(0, origin[1] + layer[1] * spacing[1], 0);
	resliceAxial->SetInterpolationModeToCubic();

	// Bu kısım için görüntü kalitesini arttıracak filter kullanılabilir. Örn: vtkImageMedian3D

	colorAxial = vtkSmartPointer<vtkImageMapToColors>::New();
	colorAxial->SetInputConnection(resliceAxial->GetOutputPort());
	colorAxial->SetLookupTable(colorFunc);
	colorAxial->Update();

	mapperAxial = vtkSmartPointer<vtkImageMapper>::New();
	mapperAxial->SetInputConnection(colorAxial->GetOutputPort());
	mapperAxial->SetColorWindow(window);
	mapperAxial->SetColorLevel(level);

	actorAxial = vtkSmartPointer<vtkActor2D>::New();
	actorAxial->SetMapper(mapperAxial);

	rendererAxial = vtkSmartPointer<vtkRenderer>::New();
	rendererAxial->AddActor(actorAxial);
	rendererAxial->SetBackground(0, 0, 0);
	rendererAxial->SetViewport(0, 0.5, 0.5, 1);

	renderWindow->AddRenderer(rendererAxial);
}

void Model::initSagittal(vtkAlgorithmOutput* input) {
	resliceSagittal = vtkSmartPointer<vtkImageReslice>::New();
	resliceSagittal->SetInputConnection(input);
	resliceSagittal->SetOutputDimensionality(2);
	resliceSagittal->SetResliceAxesDirectionCosines(0, 1, 0, 0, 0, 1, 1, 0, 0);
	resliceSagittal->SetResliceAxesOrigin(origin[0] + layer[0] * spacing[0], 0, 0);
	resliceSagittal->SetInterpolationModeToCubic();

	// Bu kısım için görüntü kalitesini arttıracak filter kullanılabilir. Örn: vtkImageMedian3D

	colorSagittal = vtkSmartPointer<vtkImageMapToColors>::New();
	colorSagittal->SetInputConnection(resliceSagittal->GetOutputPort());
	colorSagittal->SetLookupTable(colorFunc);
	colorSagittal->Update();

	mapperSagittal = vtkSmartPointer<vtkImageMapper>::New();
	mapperSagittal->SetInputConnection(colorSagittal->GetOutputPort());
	mapperSagittal->SetColorWindow(window);
	mapperSagittal->SetColorLevel(level);

	actorSagittal = vtkSmartPointer<vtkActor2D>::New();
	actorSagittal->SetMapper(mapperSagittal);

	rendererSagittal = vtkSmartPointer<vtkRenderer>::New();
	rendererSagittal->AddActor(actorSagittal);
	rendererSagittal->SetBackground(0, 0, 0);
	rendererSagittal->SetViewport(0.5, 0.5, 1, 1);

	renderWindow->AddRenderer(rendererSagittal);
}

void Model::initFrontal(vtkAlgorithmOutput* input) {
	resliceFrontal = vtkSmartPointer<vtkImageReslice>::New();
	resliceFrontal->SetInputConnection(input);
	resliceFrontal->SetOutputDimensionality(2);
	resliceFrontal->SetResliceAxesDirectionCosines(1, 0, 0, 0, 1, 0, 0, 0, 1);
	resliceFrontal->SetResliceAxesOrigin(0, 0, origin[2] + layer[2] * spacing[2]);
	resliceFrontal->SetInterpolationModeToCubic();

	// Bu kısım için görüntü kalitesini arttıracak filter kullanılabilir. Örn: vtkImageMedian3D

	colorFrontal = vtkSmartPointer<vtkImageMapToColors>::New();
	colorFrontal->SetInputConnection(resliceFrontal->GetOutputPort());
	colorFrontal->SetLookupTable(colorFunc);
	colorFrontal->Update();

	mapperFrontal = vtkSmartPointer<vtkImageMapper>::New();
	mapperFrontal->SetInputConnection(colorFrontal->GetOutputPort());
	mapperFrontal->SetColorWindow(window);
	mapperFrontal->SetColorLevel(level);

	actorFrontal = vtkSmartPointer<vtkActor2D>::New();
	actorFrontal->SetMapper(mapperFrontal);

	rendererFrontal = vtkSmartPointer<vtkRenderer>::New();
	rendererFrontal->AddActor(actorFrontal);
	rendererFrontal->SetBackground(0, 0, 0);
	rendererFrontal->SetViewport(0, 0, 0.5, 0.5);

	renderWindow->AddRenderer(rendererFrontal);
}

void Model::initVolume(vtkAlgorithmOutput* input) {
	mapperVolume = vtkSmartPointer<vtkSmartVolumeMapper>::New();
	mapperVolume->SetInputConnection(input);
	mapperVolume->SetRequestedRenderModeToGPU();

	propertyVolume = vtkSmartPointer<vtkVolumeProperty>::New();
	propertyVolume->SetColor(colorFunc);
	propertyVolume->SetScalarOpacity(opacityFunc);

	actorVolume = vtkSmartPointer<vtkVolume>::New();
	actorVolume->SetMapper(mapperVolume);
	actorVolume->SetProperty(propertyVolume);

	rendererVolume = vtkSmartPointer<vtkRenderer>::New();
	rendererVolume->AddActor(actorVolume);
	rendererVolume->SetBackground(0, 0, 0);
	rendererVolume->SetViewport(0.5, 0, 1, 0.5);

	renderWindow->AddRenderer(rendererVolume);
}

void Model::setRenderWindow() {
	if (isMax) {
		width = renderWindow->GetSize()[0];
		height = renderWindow->GetSize()[1];
	}
	else {
		width = renderWindow->GetSize()[0] / 2;
		height = renderWindow->GetSize()[1] / 2;
	}

	ratio[0] = height / (double)(extent[3] - extent[2]);
	resliceSagittal->SetOutputExtent((int)(motions[1] * ratio[0] * zooms[0]) - (int)((zooms[0] - 1) * (width / 2)),
		(int)(width + motions[1] * ratio[0] * zooms[0]) + (int)((zooms[0] - 1) * (width / 2)),
		(int)(motions[2] * ratio[0] * zooms[0]) - (int)((zooms[0] - 1) * (height / 2)),
		(int)(height + motions[2] * ratio[0] * zooms[0]) + (int)((zooms[0] - 1) * (height / 2)), 0, 0);
	resliceSagittal->SetOutputSpacing(spacing[0] / ratio[0] / zooms[0], spacing[1] / ratio[0] / zooms[0], spacing[2] / ratio[0] / zooms[0]);

	ratio[1] = height / (double)(extent[5] - extent[4]);
	resliceAxial->SetOutputExtent((int)(motions[0] * ratio[1] * zooms[1]) - (int)((zooms[1] - 1) * (width / 2)),
		(int)(width + motions[0] * ratio[1] * zooms[1]) + (int)((zooms[1] - 1) * (width / 2)),
		(int)(motions[2] * ratio[1] * zooms[1]) - (int)((zooms[1] - 1) * (height / 2)),
		(int)(height + motions[2] * ratio[1] * zooms[1]) + (int)((zooms[1] - 1) * (height / 2)), 0, 0);
	resliceAxial->SetOutputSpacing(spacing[0] / ratio[1] / zooms[1], spacing[1] / ratio[1] / zooms[1], spacing[2] / ratio[1] / zooms[1]);

	ratio[2] = height / (double)(extent[3] - extent[2]);
	resliceFrontal->SetOutputExtent((int)(motions[0] * ratio[2] * zooms[2]) - (int)((zooms[2] - 1) * (width / 2)),
		(int)(width + motions[0] * ratio[2] * zooms[2]) + (int)((zooms[2] - 1) * (width / 2)),
		(int)(motions[1] * ratio[2] * zooms[2]) - (int)((zooms[2] - 1) * (height / 2)),
		(int)(height + motions[1] * ratio[2] * zooms[2]) + (int)((zooms[2] - 1) * (height / 2)), 0, 0);
	resliceFrontal->SetOutputSpacing(spacing[0] / ratio[2] / zooms[2], spacing[1] / ratio[2] / zooms[2], spacing[2] / ratio[2] / zooms[2]);

	exts[0] = extent[5] * (int)height / (extent[3] - extent[2]);
	offsets[0] = ((int)width - exts[0]) / 2;
	exts[1] = extent[1] * (int)height / (extent[5] - extent[4]);
	offsets[1] = ((int)width - exts[1]) / 2;
	exts[2] = extent[1] * (int)height / (extent[3] - extent[2]);
	offsets[2] = ((int)width - exts[2]) / 2;
}

void Model::init2DLines() {
	 // X Eksen Çizgileri  0 --> Y / 1 --> Z
	linePos2D[0][0][0] = layer[0] * ratio[1] + offsets[1];
	linePos2D[0][0][1] = 0;
	linePos2D[0][1][0] = layer[0] * ratio[1] + offsets[1];
	linePos2D[0][1][1] = height;

	linePos2D[1][0][0] = layer[0] * ratio[2] + offsets[2];
	linePos2D[1][0][1] = 0;
	linePos2D[1][1][0] = layer[0] * ratio[2] + offsets[2];
	linePos2D[1][1][1] = height;

	// Y Eksen Çizgileri 2 --> Z / 3 --> X
	linePos2D[2][0][0] = offsets[2];
	linePos2D[2][0][1] = layer[1] * ratio[2];
	linePos2D[2][1][0] = width - offsets[2];
	linePos2D[2][1][1] = layer[1] * ratio[2];

	linePos2D[3][0][0] = layer[1] * ratio[0] + offsets[0];
	linePos2D[3][0][1] = 0;
	linePos2D[3][1][0] = layer[1] * ratio[0] + offsets[0];
	linePos2D[3][1][1] = height;

	// Z Eksen Çizgileri 4 --> Y / 5 --> X
	linePos2D[4][0][0] = offsets[1];
	linePos2D[4][0][1] = layer[2] * ratio[1];
	linePos2D[4][1][0] = width - offsets[1];
	linePos2D[4][1][1] = layer[2] * ratio[1];

	linePos2D[5][0][0] = offsets[0];
	linePos2D[5][0][1] = layer[2] * ratio[0];
	linePos2D[5][1][0] = width - offsets[0];
	linePos2D[5][1][1] = layer[2] * ratio[0];

	for (int i = 0; i < 6; i++)
	{
		lines2D[i] = vtkSmartPointer<vtkLineSource>::New();
		lines2D[i]->SetPoint1(linePos2D[i][0][0], linePos2D[i][0][1], 0);
		lines2D[i]->SetPoint2(linePos2D[i][1][0], linePos2D[i][1][1], 0);

		auto lineMapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
		auto lineActor = vtkSmartPointer<vtkActor2D>::New();
		lineMapper->SetInputConnection(lines2D[i]->GetOutputPort());
		lineActor->SetMapper(lineMapper);
		lineActor->GetProperty()->SetLineWidth(1);
		// Renk Ayarlama
		lineActor->GetProperty()->SetColor(0.5, 0.5, 0.5);
		// Panele Ekleme
		if (i == 3 || i == 5)
		{
			rendererSagittal->AddActor(lineActor);
		}
		else if (i == 0 || i == 4)
		{
			rendererAxial->AddActor(lineActor);
		}
		else if (i == 1 || i == 2)
		{
			rendererFrontal->AddActor(lineActor);
		}
	}
}

void Model::init3DLines() {
	// X-Axis Lines
	linePos3D[0][0][0] = layer[0] * spacing[0];
	linePos3D[0][1][0] = layer[0] * spacing[0];
	linePos3D[0][0][2] = 0;
	linePos3D[0][1][2] = 0;
	linePos3D[0][0][1] = 0;
	linePos3D[0][1][1] = extent[3] * spacing[1];

	linePos3D[1][0][0] = layer[0] * spacing[0];
	linePos3D[1][1][0] = layer[0] * spacing[0];
	linePos3D[1][0][2] = extent[5] * spacing[2];
	linePos3D[1][1][2] = extent[5] * spacing[2];
	linePos3D[1][0][1] = 0;
	linePos3D[1][1][1] = extent[3] * spacing[1];

	linePos3D[2][0][0] = layer[0] * spacing[0];
	linePos3D[2][1][0] = layer[0] * spacing[0];
	linePos3D[2][0][2] = 0;
	linePos3D[2][1][2] = extent[5] * spacing[2];
	linePos3D[2][0][1] = 0;
	linePos3D[2][1][1] = 0;

	linePos3D[3][0][0] = layer[0] * spacing[0];
	linePos3D[3][1][0] = layer[0] * spacing[0];
	linePos3D[3][0][2] = 0;
	linePos3D[3][1][2] = extent[5] * spacing[2];
	linePos3D[3][0][1] = extent[3] * spacing[1];
	linePos3D[3][1][1] = extent[3] * spacing[1];

	// Y-Axis Lines
	linePos3D[4][0][0] = 0;
	linePos3D[4][1][0] = extent[1] * spacing[0];
	linePos3D[4][0][2] = 0;
	linePos3D[4][1][2] = 0;
	linePos3D[4][0][1] = layer[1] * spacing[1];
	linePos3D[4][1][1] = layer[1] * spacing[1];

	linePos3D[5][0][0] = extent[1] * spacing[0];
	linePos3D[5][1][0] = extent[1] * spacing[0];
	linePos3D[5][0][2] = 0;
	linePos3D[5][1][2] = extent[5] * spacing[2];
	linePos3D[5][0][1] = layer[1] * spacing[1];
	linePos3D[5][1][1] = layer[1] * spacing[1];

	linePos3D[6][0][0] = extent[1] * spacing[0];
	linePos3D[6][1][0] = 0;
	linePos3D[6][0][2] = extent[5] * spacing[2];
	linePos3D[6][1][2] = extent[5] * spacing[2];
	linePos3D[6][0][1] = layer[1] * spacing[1];
	linePos3D[6][1][1] = layer[1] * spacing[1];

	linePos3D[7][0][0] = 0;
	linePos3D[7][1][0] = 0;
	linePos3D[7][0][2] = extent[5] * spacing[2];
	linePos3D[7][1][2] = 0;
	linePos3D[7][0][1] = layer[1] * spacing[1];
	linePos3D[7][1][1] = layer[1] * spacing[1];

	// Z-Axis Lines
	linePos3D[8][0][0] = 0;
	linePos3D[8][1][0] = 0;
	linePos3D[8][0][2] = layer[2] * spacing[2];
	linePos3D[8][1][2] = layer[2] * spacing[2];
	linePos3D[8][0][1] = 0;
	linePos3D[8][1][1] = extent[3] * spacing[1];

	linePos3D[9][0][0] = extent[1] * spacing[0];
	linePos3D[9][1][0] = extent[1] * spacing[0];
	linePos3D[9][0][2] = layer[2] * spacing[2];
	linePos3D[9][1][2] = layer[2] * spacing[2];
	linePos3D[9][0][1] = 0;
	linePos3D[9][1][1] = extent[3] * spacing[1];

	linePos3D[10][0][0] = 0;
	linePos3D[10][1][0] = extent[1] * spacing[0];
	linePos3D[10][0][2] = layer[2] * spacing[2];
	linePos3D[10][1][2] = layer[2] * spacing[2];
	linePos3D[10][0][1] = 0;
	linePos3D[10][1][1] = 0;

	linePos3D[11][0][0] = 0;
	linePos3D[11][1][0] = extent[1] * spacing[0];
	linePos3D[11][0][2] = layer[2] * spacing[2];
	linePos3D[11][1][2] = layer[2] * spacing[2];
	linePos3D[11][0][1] = extent[3] * spacing[1];
	linePos3D[11][1][1] = extent[3] * spacing[1];

	// Common intersection lines
	// Y-Z Common
	linePos3D[12][0][0] = 0;
	linePos3D[12][1][0] = extent[1] * spacing[0];
	linePos3D[12][0][2] = layer[2] * spacing[2];
	linePos3D[12][1][2] = layer[2] * spacing[2];
	linePos3D[12][0][1] = layer[1] * spacing[1];
	linePos3D[12][1][1] = layer[1] * spacing[1];

	// X-Z Common
	linePos3D[13][0][0] = layer[0] * spacing[0];
	linePos3D[13][1][0] = layer[0] * spacing[0];
	linePos3D[13][0][2] = layer[2] * spacing[2];
	linePos3D[13][1][2] = layer[2] * spacing[2];
	linePos3D[13][0][1] = 0;
	linePos3D[13][1][1] = extent[3] * spacing[1];

	// X-Y Common
	linePos3D[14][0][0] = layer[0] * spacing[0];
	linePos3D[14][1][0] = layer[0] * spacing[0];
	linePos3D[14][0][1] = layer[1] * spacing[1];
	linePos3D[14][1][1] = layer[1] * spacing[1];
	linePos3D[14][0][2] = 0;
	linePos3D[14][1][2] = extent[5] * spacing[2];

	for (int i = 0; i < 15; i++) {
		lines3D[i] = vtkSmartPointer<vtkLineSource>::New();
		lines3D[i]->SetPoint1(linePos3D[i][0][0], linePos3D[i][0][1], linePos3D[i][0][2]);
		lines3D[i]->SetPoint2(linePos3D[i][1][0], linePos3D[i][1][1], linePos3D[i][1][2]);

		auto lineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		auto lineActor = vtkSmartPointer<vtkActor>::New();
		lineMapper->SetInputConnection(lines3D[i]->GetOutputPort());
		lineActor->SetMapper(lineMapper);

		// Setting color based on the line index
		lineActor->GetProperty()->SetColor(0.5, 0.5, 0.5);

		rendererVolume->AddActor(lineActor);
	}
}

void Model::XLayer(int isUp) {
	layer[0] += isUp;

	resliceSagittal->SetResliceAxesOrigin((layer[0] * spacing[0]) + origin[0], 0, 0);

	linePos2D[0][0][0] = (layer[0] + motions[0]) * ratio[1] + offsets[1];
	linePos2D[0][0][1] = (motions[2] * ratio[1] * zooms[1]) - (height / 2 * (zooms[1] - 1));
	linePos2D[0][1][0] = (layer[0] + motions[0]) * ratio[1] + offsets[1];
	linePos2D[0][1][1] = (motions[2] * ratio[1] * zooms[1]) + height + (height / 2 * (zooms[1] - 1));
	linePos2D[1][0][0] = (layer[0] + motions[0]) * ratio[2] + offsets[2];
	linePos2D[1][0][1] = (motions[1] * ratio[2] * zooms[2]) - (height / 2 * (zooms[2] - 1));
	linePos2D[1][1][0] = (layer[0] + motions[0]) * ratio[2] + offsets[2];
	linePos2D[1][1][1] = (motions[1] * ratio[2] * zooms[2]) + height + (height / 2 * (zooms[2] - 1));

	lines2D[0]->SetPoint1(linePos2D[0][0][0], linePos2D[0][0][1], 0);
	lines2D[0]->SetPoint2(linePos2D[0][1][0], linePos2D[0][1][1], 0);
	lines2D[1]->SetPoint1(linePos2D[1][0][0], linePos2D[1][0][1], 0);
	lines2D[1]->SetPoint2(linePos2D[1][1][0], linePos2D[1][1][1], 0);

	for (int i = 0; i < 4; i++) {
		linePos3D[i][0][0] = layer[0] * spacing[0];
		linePos3D[i][1][0] = layer[0] * spacing[0];
	}

	lines3D[0]->SetPoint1(linePos3D[0][0][0], linePos3D[0][0][1], linePos3D[0][0][2]);
	lines3D[0]->SetPoint2(linePos3D[0][1][0], linePos3D[0][1][1], linePos3D[0][1][2]);
	lines3D[1]->SetPoint1(linePos3D[1][0][0], linePos3D[1][0][1], linePos3D[1][0][2]);
	lines3D[1]->SetPoint2(linePos3D[1][1][0], linePos3D[1][1][1], linePos3D[1][1][2]);
	lines3D[2]->SetPoint1(linePos3D[2][0][0], linePos3D[2][0][1], linePos3D[2][0][2]);
	lines3D[2]->SetPoint2(linePos3D[2][1][0], linePos3D[2][1][1], linePos3D[2][1][2]);
	lines3D[3]->SetPoint1(linePos3D[3][0][0], linePos3D[3][0][1], linePos3D[3][0][2]);
	lines3D[3]->SetPoint2(linePos3D[3][1][0], linePos3D[3][1][1], linePos3D[3][1][2]);

	lines3D[14]->SetPoint1(linePos3D[14][0][0], linePos3D[14][0][1], linePos3D[14][0][2]);
	lines3D[14]->SetPoint2(linePos3D[14][1][0], linePos3D[14][1][1], linePos3D[14][1][2]);
	lines3D[13]->SetPoint1(linePos3D[13][0][0], linePos3D[13][0][1], linePos3D[13][0][2]);
	lines3D[13]->SetPoint2(linePos3D[13][1][0], linePos3D[13][1][1], linePos3D[13][1][2]);
}

void Model::YLayer(int isUp) {
	layer[1] += isUp;

	resliceAxial->SetResliceAxesOrigin(0, (layer[1] * spacing[1]) + origin[1], 0);

	linePos2D[2][0][0] = (offsets[2] + motions[0] * ratio[2]) * zooms[2] - ((width / 2) * (zooms[2] - 1));
	linePos2D[2][0][1] = layer[1] + motions[1] * ratio[2];
	linePos2D[2][1][0] = width + ((width / 2) * (zooms[2] - 1)) - (offsets[2] * zooms[2]) + (motions[0] * ratio[2] * zooms[2]);
	linePos2D[2][1][1] = layer[1] + motions[1] * ratio[2];
	linePos2D[3][0][0] = (layer[1] + motions[1]) * ratio[0] + offsets[0];
	linePos2D[3][0][1] = (motions[2] * ratio[0] * zooms[0]) - (height / 2 * (zooms[0] - 1));
	linePos2D[3][1][0] = (layer[1] + motions[1]) * ratio[0] + offsets[0];
	linePos2D[3][1][1] = (motions[2] * ratio[0] * zooms[0]) + height + (height / 2 * (zooms[0] - 1));

	lines2D[2]->SetPoint1(linePos2D[2][0][0], linePos2D[2][0][1], 0);
	lines2D[2]->SetPoint2(linePos2D[2][1][0], linePos2D[2][1][1], 0);
	lines2D[3]->SetPoint1(linePos2D[3][0][0], linePos2D[3][0][1], 0);
	lines2D[3]->SetPoint2(linePos2D[3][1][0], linePos2D[3][1][1], 0);

	for (int i = 4; i <= 7; i++) {
		linePos3D[i][0][1] = layer[1] * spacing[1];
		linePos3D[i][1][1] = layer[1] * spacing[1];
	}

	linePos3D[12][0][1] = layer[1] * spacing[1];
	linePos3D[12][1][1] = layer[1] * spacing[1];
	linePos3D[14][0][1] = layer[1] * spacing[1];
	linePos3D[14][1][1] = layer[1] * spacing[1];

	lines3D[4]->SetPoint1(linePos3D[4][0][0], linePos3D[4][0][1], linePos3D[4][0][2]);
	lines3D[4]->SetPoint2(linePos3D[4][1][0], linePos3D[4][1][1], linePos3D[4][1][2]);
	lines3D[5]->SetPoint1(linePos3D[5][0][0], linePos3D[5][0][1], linePos3D[5][0][2]);
	lines3D[5]->SetPoint2(linePos3D[5][1][0], linePos3D[5][1][1], linePos3D[5][1][2]);
	lines3D[6]->SetPoint1(linePos3D[6][0][0], linePos3D[6][0][1], linePos3D[6][0][2]);
	lines3D[6]->SetPoint2(linePos3D[6][1][0], linePos3D[6][1][1], linePos3D[6][1][2]);
	lines3D[7]->SetPoint1(linePos3D[7][0][0], linePos3D[7][0][1], linePos3D[7][0][2]);
	lines3D[7]->SetPoint2(linePos3D[7][1][0], linePos3D[7][1][1], linePos3D[7][1][2]);

	lines3D[12]->SetPoint1(linePos3D[12][0][0], linePos3D[12][0][1], linePos3D[12][0][2]);
	lines3D[12]->SetPoint2(linePos3D[12][1][0], linePos3D[12][1][1], linePos3D[12][1][2]);
	lines3D[14]->SetPoint1(linePos3D[14][0][0], linePos3D[14][0][1], linePos3D[14][0][2]);
	lines3D[14]->SetPoint2(linePos3D[14][1][0], linePos3D[14][1][1], linePos3D[14][1][2]);
}

void Model::ZLayer(int isUp) {
	layer[2] += isUp;

	resliceFrontal->SetResliceAxesOrigin(0, 0, (layer[2] * spacing[2]) + origin[2]);

	linePos2D[4][0][0] = (offsets[1] + motions[0] * ratio[1]) * zooms[1] - ((width / 2) * (zooms[1] - 1));
	linePos2D[4][0][1] = (layer[2] + motions[2]) * ratio[1];
	linePos2D[4][1][0] = width + ((width / 2) * (zooms[1] - 1)) - (offsets[1] * zooms[1]) + (motions[0] * ratio[1] * zooms[1]);
	linePos2D[4][1][1] = (layer[2] + motions[2]) * ratio[1];

	linePos2D[5][0][0] = (offsets[0] + motions[1] * ratio[0]) * zooms[0] - ((width / 2) * (zooms[0] - 1));
	linePos2D[5][0][1] = (layer[2] + motions[2]) * ratio[0];
	linePos2D[5][1][0] = width + ((width / 2) * (zooms[0] - 1)) - (offsets[0] * zooms[0]) + (motions[1] * ratio[0] * zooms[0]);
	linePos2D[5][1][1] = (layer[2] + motions[2]) * ratio[0];

	lines2D[4]->SetPoint1(linePos2D[4][0][0], linePos2D[4][0][1], 0);
	lines2D[4]->SetPoint2(linePos2D[4][1][0], linePos2D[4][1][1], 0);
	lines2D[5]->SetPoint1(linePos2D[5][0][0], linePos2D[5][0][1], 0);
	lines2D[5]->SetPoint2(linePos2D[5][1][0], linePos2D[5][1][1], 0);

	for (int i = 8; i <= 13; i++) {
		linePos3D[i][0][2] = layer[2] * spacing[2];
		linePos3D[i][1][2] = layer[2] * spacing[2];
	}

	lines3D[8]->SetPoint1(linePos3D[8][0][0], linePos3D[8][0][1], linePos3D[8][0][2]);
	lines3D[8]->SetPoint2(linePos3D[8][1][0], linePos3D[8][1][1], linePos3D[8][1][2]);
	lines3D[9]->SetPoint1(linePos3D[9][0][0], linePos3D[9][0][1], linePos3D[9][0][2]);
	lines3D[9]->SetPoint2(linePos3D[9][1][0], linePos3D[9][1][1], linePos3D[9][1][2]);
	lines3D[10]->SetPoint1(linePos3D[10][0][0], linePos3D[10][0][1], linePos3D[10][0][2]);
	lines3D[10]->SetPoint2(linePos3D[10][1][0], linePos3D[10][1][1], linePos3D[10][1][2]);
	lines3D[11]->SetPoint1(linePos3D[11][0][0], linePos3D[11][0][1], linePos3D[11][0][2]);
	lines3D[11]->SetPoint2(linePos3D[11][1][0], linePos3D[11][1][1], linePos3D[11][1][2]);

	lines3D[12]->SetPoint1(linePos3D[12][0][0], linePos3D[12][0][1], linePos3D[12][0][2]);
	lines3D[12]->SetPoint2(linePos3D[12][1][0], linePos3D[12][1][1], linePos3D[12][1][2]);
	lines3D[13]->SetPoint1(linePos3D[13][0][0], linePos3D[13][0][1], linePos3D[13][0][2]);
	lines3D[13]->SetPoint2(linePos3D[13][1][0], linePos3D[13][1][1], linePos3D[13][1][2]);
}

void Model::XLayerMotion(int isUp) {
	motions[0] -= isUp;
	XLayer(isUp);
	YLayer(0);
	ZLayer(0);
	setRenderWindow();
}

void Model::YLayerMotion(int isUp) {
	motions[1] -= isUp;
	YLayer(isUp);
	XLayer(0);
	ZLayer(0);
	setRenderWindow();
}

void Model::ZLayerMotion(int isUp) {
	motions[2] -= isUp;
	ZLayer(isUp);
	XLayer(0);
	YLayer(0);
	setRenderWindow();
}

void Model::SetPanel() {
	setRenderWindow();
	XLayer(0);
	YLayer(0);
	ZLayer(0);
}

void Model::Maximize(int panel) {
	if (isMax) {
		rendererAxial->SetViewport(0, 0.5, 0.5, 1);
		rendererSagittal->SetViewport(0.5, 0.5, 1, 1);
		rendererFrontal->SetViewport(0, 0, 0.5, 0.5);
		rendererVolume->SetViewport(0.5, 0, 1, 0.5);
	} else {
		if (panel == 1) {
			rendererAxial->SetViewport(0, 0, 1, 1);
			rendererSagittal->SetViewport(0, 0, 0, 0);
			rendererFrontal->SetViewport(0, 0, 0, 0);
			rendererVolume->SetViewport(0, 0, 0, 0);
		} else if (panel == 2) {
			rendererAxial->SetViewport(0, 0, 0, 0);
			rendererSagittal->SetViewport(0, 0, 1, 1);
			rendererFrontal->SetViewport(0, 0, 0, 0);
			rendererVolume->SetViewport(0, 0, 0, 0);
		} else if (panel == 3) {
			rendererAxial->SetViewport(0, 0, 0, 0);
			rendererSagittal->SetViewport(0, 0, 0, 0);
			rendererFrontal->SetViewport(0, 0, 1, 1);
			rendererVolume->SetViewport(0, 0, 0, 0);
		} else if (panel == 4) {
			rendererAxial->SetViewport(0, 0, 0, 0);
			rendererSagittal->SetViewport(0, 0, 0, 0);
			rendererFrontal->SetViewport(0, 0, 0, 0);
			rendererVolume->SetViewport(0, 0, 1, 1);
		}
	}
}

void Model::ChangeLayer(int panel, int dir) {
	if (dir == 1) {
		if (panel == 0) {
			if (layer[0] < extent[1]) {
				XLayerMotion(1);
			}
		} else if (panel == 1) {
			if (layer[1] < extent[3]) {
				YLayerMotion(1);
			}
		} else if (panel == 2) {
			if (layer[2] < extent[5]) {
				ZLayerMotion(1);
			}
		}
	} else if (dir == -1) {
		if (panel == 0) {
			if (layer[0] > extent[0]) {
				XLayerMotion(-1);
			}
		} else if (panel == 1) {
			if (layer[1] > extent[2]) {
				YLayerMotion(-1);
			}
		} else if (panel == 2) {
			if (layer[2] > extent[4]) {
				ZLayerMotion(-1);
			}
		}
	}
}

void Model::ChangeMotion(int panel, int dir) {
	if (dir == 1) {
		if (panel == 0) {
			if (layer[2] < extent[5]) {
				ZLayerMotion(1);
			}
		} else if (panel == 1) {
			if (layer[2] < extent[5]) {
				ZLayerMotion(1);
			}
		} else if (panel == 2) {
			if (layer[1] < extent[3]) {
				YLayerMotion(1);
			}
		} 
	} else if (dir == 2) {
		if (panel == 0) {
			if (layer[2] > extent[4]) {
				ZLayerMotion(-1);
			}
		} else if (panel == 1) {
			if (layer[2] > extent[4]) {
				ZLayerMotion(-1);
			}
		} else if (panel == 2) {
			if (layer[1] > extent[2]) {
				YLayerMotion(-1);
			}
		} 
	} else if (dir == 3) {
		if (panel == 0) {
			if (layer[0] > extent[0]) {
				XLayerMotion(-1);
			}
		} else if (panel == 1) {
			if (layer[1] > extent[2]) {
				YLayerMotion(-1);
			}
		} else if (panel == 2) {
			if (layer[0] > extent[0]) {
				XLayerMotion(-1);
			}
		} 
	} else if (dir == 4) {
		if (panel == 0) {
			if (layer[0] < extent[1]) {
				XLayerMotion(1);
			}
		} else if (panel == 1) {
			if (layer[1] < extent[3]) {
				YLayerMotion(1);
			}
		} else if (panel == 2) {
			if (layer[0] < extent[1]) {
				XLayerMotion(1);
			}
		} 
	}
}

void Model::ChangeZoom(int panel, double value) {
	zooms[panel] = value;
	SetPanel();
}

void Model::Reset() {
	zooms[0] = 1;
	zooms[1] = 1;
	zooms[2] = 1;
	motions[0] = 0;
	motions[1] = 0;
	motions[2] = 0;
	isMax = false;
	layer[0] = (extent[1] - extent[0]) / 2;
	layer[1] = (extent[3] - extent[2]) / 2;
	layer[2] = (extent[5] - extent[4]) / 2;
	SetPanel();
}
#pragma endregion

VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2);
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

int main()
{
	Model model;
	vtkAlgorithmOutput* output = model.readDicom("/home/inferna/Belgeler/Dicoms/beyin");
	model.initAxial(output);
	model.initSagittal(output);
	model.initFrontal(output);
	model.initVolume(output);
	model.setRenderWindow();
	model.init2DLines();
	model.init3DLines();
	model.start();
	
	return 0;
}