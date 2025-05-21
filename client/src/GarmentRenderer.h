#include <QQuickItem>
#include <QOpenGLShaderProgram>

class GarmentRenderer : public QQuickItem {
    Q_OBJECT
public:
    GarmentRenderer();
    
public slots:
    void updateMesh(const Mesh& mesh);

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*);

private:
    Mesh m_currentMesh;
    QOpenGLShaderProgram m_shaderProgram;
};