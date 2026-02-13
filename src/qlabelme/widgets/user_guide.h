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

private:
    void buildContents();
    void showPage(const QString& key);

private:
    Ui::UserGuide *ui;
};
