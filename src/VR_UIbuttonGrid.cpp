#include "VR_UIbuttonGrid.h"
#include <cnoid/MeshGenerator>
#include <cnoid/MessageView>
#include <cnoid/RootItem>
#include <cnoid/EigenTypes>
#include <cnoid/SceneDrawables>
#include <cnoid/SceneItem>
#include <cnoid/SceneView>
#include "../../openvr_plugin/src/OpenVRPlugin.h"
using namespace cnoid;

VR_UIbuttonGrid::VR_UIbuttonGrid(std::string NAME ,int rows, int cols, double spacing, const Vector3& buttonSize): NAME(NAME),rows(rows), cols(cols), spacing(spacing), buttonSize(buttonSize) {
    buttonGroup = new SgSwitchableGroup();
    offset_to_camera.pos = Vector3(0,0,0);
    offset_to_camera.rot <<     1, 0,  0,
                                0, 1,  0,
                                0, 0,  1;
}

void VR_UIbuttonGrid::createButtons(const std::vector<std::string>& buttonNames, const std::vector<std::string>& texturePaths) {
    MeshGenerator generator;
    RootItemPtr root = RootItem::instance();
    auto* mv = MessageView::instance(); // MessageViewを使用してエラーメッセージを出力

    int index = 0;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            // ボタンのSceneItemを作成
            SceneItemPtr buttonItem = new SceneItem();
            buttonItem->setName(buttonNames[index]);

            // ボタンの形状
            SgShape* shape = new SgShape();
            shape->setMesh(generator.generateBox(buttonSize));

            // マテリアル設定（単色）
            SgMaterial* material = shape->getOrCreateMaterial();
            material->setDiffuseColor(Vector3f(1.0f, 1.0f, 1.0f)); // 白色
            material->setAmbientIntensity(1.0f);
            material->setTransparency(0.0f);     // 透明度を0に設定（完全不透明）

            // UVマッピング設定
            SgMesh* mesh = shape->getOrCreateMesh();
            SgTexCoordArray* texCoords = new SgTexCoordArray();
            texCoords->resize(4);
            (*texCoords)[0] = Vector2f(0.0f, 0.0f);
            (*texCoords)[1] = Vector2f(1.0f, 0.0f);
            (*texCoords)[2] = Vector2f(1.0f, 1.0f);
            (*texCoords)[3] = Vector2f(0.0f, 1.0f);
            mesh->setTexCoords(texCoords);
            // テクスチャ設定
            SgTexture* texture = new SgTexture();
            SgImagePtr sgImage = new SgImage();

            // 画像をロード
            if (sgImage->image().load(texturePaths[index])) {
                sgImage->image().applyVerticalFlip(); // ここで画像を垂直方向に反転
                texture->setImage(sgImage); // テクスチャに画像を設定
                shape->setTexture(texture); // SgShapeにテクスチャを設定
            } else {
                mv->putln("Failed to load texture: " + texturePaths[index], MessageView::WARNING); // デバッグ通知
            }
            
            // ボタンの位置
            SgPosTransform* buttonTransform = new SgPosTransform();
            buttonTransform->setTranslation(Vector3(j * spacing, -i * spacing, 0.0));
            buttonTransform->addChild(shape);

            // グループに追加
            buttonGroup->addChild(buttonTransform);

            // ボタンのSceneItemとTransformをペアとして記録
            buttons.emplace_back(buttonItem, buttonTransform);

            index++;
        }
    }

    // SceneItemにグループをまとめて追加
    SceneItemPtr sceneItem = new SceneItem();
    sceneItem->setName(NAME);
    sceneItem->topNode()->addChild(buttonGroup);
    sceneItem->setChecked(true);
    root->addChildItem(sceneItem);
}

void VR_UIbuttonGrid::setVisible(bool visible) {
    if(buttonGroup) {
        buttonGroup->setTurnedOn(visible);
    }
}

void VR_UIbuttonGrid::setVisible_image(bool visible) {
    if (description_image.first && description_image.second) {
        description_image.first->setChecked(visible); // SceneItemの表示状態を変更
    } 
}


void cnoid::VR_UIbuttonGrid::setPosition(const coordinates& coords) {
    // グリッド全体の座標を更新
    gridCoords = coords;
    coordinates setcoords = coords;
    // グリッド全体のトランスフォームを適用
    SgPosTransform* groupTransform = new SgPosTransform();
    groupTransform->setTranslation(setcoords.pos);
    groupTransform->setRotation(setcoords.rot);

    // 現在のボタンを新しいトランスフォームに追加
    for (auto& [item, transform] : buttons) {
        groupTransform->addChild(transform);
    }

    // buttonGroup の子ノードを新しいトランスフォームに置き換える
    buttonGroup->clearChildren();
    buttonGroup->addChild(groupTransform);

    // descriptionImageItem の位置も更新する
    if (description_image.first && description_image.second) {
        SgPosTransform* descriptionTransform = description_image.second;

        // descriptionImageItem の相対位置をgridCoordsを利用して計算
        double centerX = cols * spacing / 2.0 - spacing / 2.0; // ボタン群の中央 (X軸)
        double centerY = rows * spacing - 0.95;                // ボタン群の上端 (Y軸)に適当なオフセット
        Vector3 descriptionOffset(centerX, centerY, 0.0);     // イメージ画像のローカルオフセット位置

        // グリッド座標系を基にグローバル位置を計算
        Vector3 globalPosition = gridCoords.pos + gridCoords.rot * descriptionOffset;

        // descriptionImageItem のトランスフォームを更新
        descriptionTransform->setTranslation(globalPosition);
        descriptionTransform->setRotation(gridCoords.rot);
    }
}

void VR_UIbuttonGrid::setSelectedButton(const SceneItemPtr& selectedButton) {
    // 選択解除するために、現在の選択ボタンをリセット
    if (currentSelectedButton) {
        for (auto& [item, transform] : buttons) {
            if (item == currentSelectedButton) {
                // 元の色に戻す
                SgShape* shape = dynamic_cast<SgShape*>(transform->child(0));
                if (shape) {
                    SgMaterial* material = shape->getOrCreateMaterial();
                    material->setDiffuseColor(Vector3f(1.0f, 1.0f, 1.0f)); // 白色
                }
                break;
            }
        }
    }

    // 新しいボタンを選択状態にする
    currentSelectedButton = selectedButton;

    if (currentSelectedButton) {
        for (auto& [item, transform] : buttons) {
            if (item == currentSelectedButton) {
                // 色を灰色に変更
                SgShape* shape = dynamic_cast<SgShape*>(transform->child(0));
                if (shape) {
                    SgMaterial* material = shape->getOrCreateMaterial();
                    material->setDiffuseColor(Vector3f(0.5f, 0.5f, 0.5f)); // 灰色
                }
                break;
            }
        }
    }
}
void VR_UIbuttonGrid::createDescriptionImage(const std::string& texturePath, const Vector3& imageSize, double offsetY) {
    MeshGenerator generator;
    RootItemPtr root = RootItem::instance();
    auto* mv = MessageView::instance();

    // 説明用のSceneItemを作成
    SceneItemPtr descriptionItem = new SceneItem();
    descriptionItem->setName(NAME + "_Description");

    // 画像の形状を生成
    SgShape* shape = new SgShape();
    shape->setMesh(generator.generateBox(imageSize)); // 指定されたサイズのボックス形状

    // マテリアルとテクスチャを設定
    SgMaterial* material = shape->getOrCreateMaterial();
    material->setDiffuseColor(Vector3f(1.0f, 1.0f, 1.0f)); // 白色


    // UVマッピング設定
    SgMesh* mesh = shape->getOrCreateMesh();
    SgTexCoordArray* texCoords = new SgTexCoordArray();
    texCoords->resize(4);
    (*texCoords)[0] = Vector2f(0.0f, 0.0f);
    (*texCoords)[1] = Vector2f(1.0f, 0.0f);
    (*texCoords)[2] = Vector2f(1.0f, 1.0f);
    (*texCoords)[3] = Vector2f(0.0f, 1.0f);
    mesh->setTexCoords(texCoords);

    SgTexture* texture = new SgTexture();
    SgImagePtr sgImage = new SgImage();

    if (sgImage->image().load(texturePath)) {
        sgImage->image().applyVerticalFlip(); // 画像を上下反転
        texture->setImage(sgImage);
        shape->setTexture(texture);
    } else {
        mv->putln("Failed to load texture: " + texturePath, MessageView::WARNING);
        return;
    }

    // 画像の位置を設定
    SgPosTransform* transform = new SgPosTransform();
    double centerX = cols * spacing / 2.0; // ボタン群の中央
    double centerY = rows * spacing - offsetY; // ボタン群の上にオフセット
    transform->setTranslation(Vector3(centerX, centerY, 0.0));
    transform->addChild(shape);
    
    // SceneItemに追加
    descriptionItem->topNode()->addChild(transform);
    descriptionItem->setChecked(true);
    root->addChildItem(descriptionItem);

    // オブジェクトをメンバー変数として保持
    this->description_image = {descriptionItem, transform};
}
void VR_UIbuttonGrid::changeDescriptionTexture(const std::string& texturePath) {
    if (!description_image.first || !description_image.second) {
        MessageView::instance()->putln("Description image has not been created.", MessageView::WARNING);
        return;
    }

    // 説明画像の形状を取得
    SgPosTransform* transform = description_image.second;
    if (!transform) {
        MessageView::instance()->putln("Failed to get description image transform.", MessageView::WARNING);
        return;
    }

    SgShape* shape = dynamic_cast<SgShape*>(transform->child(0));
    if (!shape) {
        MessageView::instance()->putln("Failed to get description image shape.", MessageView::WARNING);
        return;
    }

    // テクスチャを取得またはロードしてキャッシュに保存
    SgImagePtr sgImage;
    if (imageCache.find(texturePath) == imageCache.end()) {
        sgImage = new SgImage();
        if (sgImage->image().load(texturePath)) {
            sgImage->image().applyVerticalFlip();  // 画像を上下反転
            imageCache[texturePath] = sgImage;     // キャッシュに保存
            MessageView::instance()->putln("Loaded texture: " + texturePath);
        } else {
            MessageView::instance()->putln("Failed to load texture: " + texturePath, MessageView::WARNING);
            return;
        }
    } else {
        sgImage = imageCache[texturePath]; // キャッシュから取得
        MessageView::instance()->putln("Using cached texture: " + texturePath);
    }

    // テクスチャを更新
    SgTexture* texture = shape->getOrCreateTexture();
    texture->setImage(sgImage);
    shape->setTexture(texture);
}

void cnoid::VR_UIbuttonGrid::updateDescriptionTexture()
{   if (!currentSelectedButton) {
        return;
    }
    std::string selectedButtonName = currentSelectedButton->name();
    std::string path = "../discription_image/" + NAME + "/" + selectedButtonName + ".png";
    changeDescriptionTexture(path);
}

void cnoid::VR_UIbuttonGrid::setPosition_camera(coordinates& camera_coords)
{
    // カメラ座標系にオフセットを適用
    coordinates gridCoords_camera;
    gridCoords_camera.pos = camera_coords.pos + camera_coords.rot * offset_to_camera.pos;
    gridCoords_camera.rot = camera_coords.rot * offset_to_camera.rot;
    
    // ボタン群を移動
    setPosition(gridCoords_camera);
}

void cnoid::VR_UIbuttonGrid::setOffsetToCamera(coordinates offset) {
     offset_to_camera = offset;
}
