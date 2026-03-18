#include <QDialog>

namespace Ui {
class UserGuide;
}

class UserGuide : public QDialog
{
    Q_OBJECT

public:
    explicit UserGuide(QWidget *parent = nullptr);
    ~UserGuide();

protected:
    void closeEvent(QCloseEvent* event) override;

public slots:
    void reject() override;

private:
    void buildContents();
    void showPage(const QString& key);

    void loadGeometry();
    void saveGeometry() const;

private:
    Ui::UserGuide *ui;
};
