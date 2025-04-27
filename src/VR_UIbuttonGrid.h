#ifndef VR_UIBUTTONGRID_H
#define VR_UIBUTTONGRID_H

#include "../../openvr_plugin/src/OpenVRPlugin.h"
#include <cnoid/SceneGraph>
#include <cnoid/SceneItem>
#include <cnoid/EigenTypes>
#include <vector>
#include <string>

namespace cnoid {

class VR_UIbuttonGrid {
public:
    VR_UIbuttonGrid(std::string NAME , int rows, int cols, double spacing, const Vector3& buttonSize);

    // ボタンを生成する
    void createButtons(const std::vector<std::string>& buttonNames,const std::vector<std::string>& texturePaths);

    // 全てのボタンを表示/非表示にする
    void setVisible(bool visible);

    // グリッド全体を移動および回転させる
    void setPosition(const coordinates& coords);

    // カメラから見てボタン群を移動させる
    void setPosition_camera(coordinates& camera_coords);

    // 選択されたボタンを設定する
    void setSelectedButton(const SceneItemPtr& selectedButton);

    void createDescriptionImage(const std::string& texturePath, const Vector3& imageSize, double offsetY);
    void changeDescriptionTexture(const std::string& texturePath);
    void updateDescriptionTexture();
    void setVisible_image(bool visible);
    // Getter for buttons
    const std::vector<std::pair<SceneItemPtr, SgPosTransform*>>& getButtons() const { return buttons; }
    const SgSwitchableGroupPtr& getButtonGroup() const { return buttonGroup; }
    const std::string& getname()const{return NAME;}
    const coordinates& getcoords()const{return gridCoords;};
    const std::string& getselected_button()const{
        if(currentSelectedButton){
            return currentSelectedButton->name();
        }
        std::string null = "";
        return null;
    }
    void setOffsetToCamera(const coordinates offset); // セッター

private:
    int rows;                            // ボタンの行数
    int cols;                            // ボタンの列数
    double spacing;                      // ボタン間の間隔

    coordinates offset_to_camera;
    std::string NAME; 
    Vector3 buttonSize;                  // ボタンのサイズ
    coordinates gridCoords;              // グリッド全体の座標
    SgSwitchableGroupPtr buttonGroup;    // ボタンをまとめるグループ
    SceneItemPtr currentSelectedButton; // 現在選択されているボタン
    SceneItemPtr descriptionImageItem; // 説明用画像のSceneItem
    SgShape* descriptionShape;           // 説明用の形状

    // ボタンのSceneItemとTransformのペアを保持する
    std::vector<std::pair<SceneItemPtr, SgPosTransform*>> buttons;
    std::pair<SceneItemPtr, SgPosTransform*> description_image;

    // 画像をキャッシュで記憶
    std::unordered_map<std::string, SgImagePtr> imageCache;
};

}

#endif // VR_UIBUTTONGRID_H