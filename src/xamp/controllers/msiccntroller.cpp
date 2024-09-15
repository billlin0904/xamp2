#include <controllers/msiccntroller.h>
#include <models/musicplayermodel.h>

MusicController::MusicController(MusicPlayerModel* model, Xamp* view, QObject* parent)
	: QObject(parent)
	, model_(model)
	, view_(view) {
}
