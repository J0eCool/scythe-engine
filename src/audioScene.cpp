#include "audioScene.h"

AudioScene::AudioScene(Allocator *alloc, Input *input)
        : _ui(alloc, input), _input(input) {
}

void AudioScene::onLoad() {
}
void AudioScene::onUnload() {
}
void AudioScene::update(float dt) {
    _ui.startUpdate({300, 100});
    _ui.label("hello WERL");
}
void AudioScene::render(Renderer *renderer) {
    renderer->background(bgColor);
    _ui.render(renderer);
}
