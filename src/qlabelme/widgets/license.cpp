#include "license.h"
#include "ui_license.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QPushButton>
#include <QTextCursor>
#include <QFontDatabase>


License::License(QWidget* parent)
    : QDialog(parent),
      ui(new Ui::License)
{
    ui->setupUi(this);

    // Моноширинный шрифт сохраняет исходное расположение отступов
    ui->licenseBrowser->setFont(
        QFontDatabase::systemFont(QFontDatabase::FixedFont));

    // Сохраняем переносы строк исходного файла
    ui->licenseBrowser->setLineWrapMode(QTextEdit::NoWrap);

    setWindowTitle(u8"Лицензия QLabelMe");
    resize(750, 550);

    // Загружаем полный текст лицензии из ресурсов приложения
    QFile file(":/LICENSE");

    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        ui->licenseBrowser->setPlainText(
            QString::fromUtf8(file.readAll()));
    }
    else
    {
        ui->licenseBrowser->setPlainText(
            u8"Не удалось загрузить текст лицензии.");
    }

    // Выравниваем все абзацы лицензии по левому краю
    ui->licenseBrowser->selectAll();
    ui->licenseBrowser->setAlignment(Qt::AlignLeft);

    // Снимаем выделение и возвращаемся к началу текста
    QTextCursor cursor = ui->licenseBrowser->textCursor();
    cursor.clearSelection();
    cursor.movePosition(QTextCursor::Start);
    ui->licenseBrowser->setTextCursor(cursor);

    ui->buttonBox->button(QDialogButtonBox::Close)->setText(u8"Закрыть");

    connect(ui->buttonBox, &QDialogButtonBox::rejected,
            this, &License::reject);
}

License::~License()
{
    delete ui;
}
