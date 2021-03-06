/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "KnobGuiString.h"

#include <cfloat>
#include <algorithm> // min, max
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QColorDialog>
#include <QToolTip>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QApplication>
#include <QScrollArea>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QtCore/QDebug>
#include <QFontComboBox>
#include <QDialogButtonBox>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveGui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/KnobWidgetDnD.h"
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"

#include "ofxNatron.h"

NATRON_NAMESPACE_ENTER;
using std::make_pair;


//=============================STRING_KNOB_GUI===================================

AnimatingTextEdit::AnimatingTextEdit(const KnobGuiPtr& knob,
                                     int dimension,
                                     QWidget* parent)
    : QTextEdit(parent)
    , animation(0)
    , readOnlyNatron(false)
    , _hasChanged(false)
    , dirty(false)
    , _dnd( KnobWidgetDnD::create(knob, dimension, this) )
{
    setTabStopWidth(20); // a tab width of 20 is more reasonable than 80 for programming languages (e.g. Shadertoy)
}

AnimatingTextEdit::~AnimatingTextEdit()
{
}

void
AnimatingTextEdit::setAnimation(int v)
{
    if (v != animation) {
        animation = v;
        style()->unpolish(this);
        style()->polish(this);
        update();
    }
}

void
AnimatingTextEdit::setReadOnlyNatron(bool ro)
{
    setReadOnly(ro);
    if (readOnlyNatron != ro) {
        readOnlyNatron = ro;
        style()->unpolish(this);
        style()->polish(this);
        update();
    }
}

void
AnimatingTextEdit::setDirty(bool b)
{
    if (dirty != b) {
        dirty = b;
        style()->unpolish(this);
        style()->polish(this);
        update();
    }
}

void
AnimatingTextEdit::enterEvent(QEvent* e)
{
    _dnd->mouseEnter(e);
    QTextEdit::enterEvent(e);
}

void
AnimatingTextEdit::leaveEvent(QEvent* e)
{
    _dnd->mouseLeave(e);
    QTextEdit::leaveEvent(e);
}

void
AnimatingTextEdit::keyPressEvent(QKeyEvent* e)
{
    _dnd->keyPress(e);
    if ( modCASIsControl(e) && (e->key() == Qt::Key_Return) ) {
        if (_hasChanged) {
            _hasChanged = false;
            Q_EMIT editingFinished();
        }
    }
    _hasChanged = true;
    QTextEdit::keyPressEvent(e);
}

void
AnimatingTextEdit::keyReleaseEvent(QKeyEvent* e)
{
    _dnd->keyRelease(e);
    QTextEdit::keyReleaseEvent(e);
}

void
AnimatingTextEdit::paintEvent(QPaintEvent* e)
{
    QPalette p = this->palette();
    QColor c(200, 200, 200, 255);

    p.setColor( QPalette::Highlight, c );
    //p.setColor( QPalette::HighlightedText, c );
    this->setPalette( p );
    QTextEdit::paintEvent(e);
}

void
AnimatingTextEdit::mousePressEvent(QMouseEvent* e)
{
    if ( !_dnd->mousePress(e) ) {
        QTextEdit::mousePressEvent(e);
    }
}

void
AnimatingTextEdit::mouseMoveEvent(QMouseEvent* e)
{
    if ( !_dnd->mouseMove(e) ) {
        QTextEdit::mouseMoveEvent(e);
    }
}

void
AnimatingTextEdit::mouseReleaseEvent(QMouseEvent* e)
{
    _dnd->mouseRelease(e);
    QTextEdit::mouseReleaseEvent(e);
}

void
AnimatingTextEdit::dragEnterEvent(QDragEnterEvent* e)
{
    if ( !_dnd->dragEnter(e) ) {
        QTextEdit::dragEnterEvent(e);
    }
}

void
AnimatingTextEdit::dragMoveEvent(QDragMoveEvent* e)
{
    if ( !_dnd->dragMove(e) ) {
        QTextEdit::dragMoveEvent(e);
    }
}

void
AnimatingTextEdit::dropEvent(QDropEvent* e)
{
    if ( !_dnd->drop(e) ) {
        QTextEdit::dropEvent(e);
    }
}

void
AnimatingTextEdit::focusInEvent(QFocusEvent* e)
{
    _dnd->focusIn();
    QTextEdit::focusInEvent(e);
}

void
AnimatingTextEdit::focusOutEvent(QFocusEvent* e)
{
    _dnd->focusOut();
    if (_hasChanged) {
        _hasChanged = false;
        Q_EMIT editingFinished();
    }
    QTextEdit::focusOutEvent(e);
}

KnobLineEdit::KnobLineEdit(const KnobGuiPtr& knob,
                           int dimension,
                           QWidget* parent)
    : LineEdit(parent)
    , _dnd( KnobWidgetDnD::create(knob, dimension, this) )
{}

KnobLineEdit::~KnobLineEdit()
{
}

void
KnobLineEdit::enterEvent(QEvent* e)
{
    _dnd->mouseEnter(e);
    LineEdit::enterEvent(e);
}

void
KnobLineEdit::leaveEvent(QEvent* e)
{
    _dnd->mouseLeave(e);
    LineEdit::leaveEvent(e);
}

void
KnobLineEdit::keyPressEvent(QKeyEvent* e)
{
    _dnd->keyPress(e);
    LineEdit::keyPressEvent(e);
}

void
KnobLineEdit::keyReleaseEvent(QKeyEvent* e)
{
    _dnd->keyRelease(e);
    LineEdit::keyReleaseEvent(e);
}

void
KnobLineEdit::mousePressEvent(QMouseEvent* e)
{
    if ( !_dnd->mousePress(e) ) {
        LineEdit::mousePressEvent(e);
    }
}

void
KnobLineEdit::mouseMoveEvent(QMouseEvent* e)
{
    if ( !_dnd->mouseMove(e) ) {
        LineEdit::mouseMoveEvent(e);
    }
}

void
KnobLineEdit::mouseReleaseEvent(QMouseEvent* e)
{
    _dnd->mouseRelease(e);
    LineEdit::mouseReleaseEvent(e);
}

void
KnobLineEdit::dragEnterEvent(QDragEnterEvent* e)
{
    if ( !_dnd->dragEnter(e) ) {
        LineEdit::dragEnterEvent(e);
    }
}

void
KnobLineEdit::dragMoveEvent(QDragMoveEvent* e)
{
    if ( !_dnd->dragMove(e) ) {
        LineEdit::dragMoveEvent(e);
    }
}

void
KnobLineEdit::dropEvent(QDropEvent* e)
{
    if ( !_dnd->drop(e) ) {
        LineEdit::dropEvent(e);
    }
}

void
KnobLineEdit::focusInEvent(QFocusEvent* e)
{
    _dnd->focusIn();
    LineEdit::focusInEvent(e);
}

void
KnobLineEdit::focusOutEvent(QFocusEvent* e)
{
    _dnd->focusOut();
    LineEdit::focusOutEvent(e);
}

KnobGuiString::KnobGuiString(KnobIPtr knob,
                             KnobGuiContainerI *container)
    : KnobGui(knob, container)
    , _lineEdit(0)
    , _label(0)
    , _container(0)
    , _mainLayout(0)
    , _textEdit(0)
    , _richTextOptions(0)
    , _richTextOptionsLayout(0)
    , _fontCombo(0)
    , _setBoldButton(0)
    , _setItalicButton(0)
    , _fontSizeSpinBox(0)
    , _fontColorButton(0)
{
    _knob = toKnobString(knob);
}

QFont
KnobGuiString::makeFontFromState() const
{
    KnobStringPtr knob = _knob.lock();
    QFont f;
    f.setFamily(QString::fromUtf8(knob->getFontFamily().c_str()));
    f.setPointSize(knob->getFontSize());
    f.setBold(knob->getBoldActivated());
    f.setItalic(knob->getItalicActivated());
    return f;
}

void
KnobGuiString::createWidget(QHBoxLayout* layout)
{
    KnobStringPtr knob = _knob.lock();

    if ( knob->isMultiLine() ) {
        _container = new QWidget( layout->parentWidget() );
        _mainLayout = new QVBoxLayout(_container);
        _mainLayout->setContentsMargins(0, 0, 0, 0);
        _mainLayout->setSpacing(0);

        bool useRichText = knob->usesRichText();
        _textEdit = new AnimatingTextEdit(shared_from_this(), 0, _container);
        _textEdit->setAcceptRichText(useRichText);


        _mainLayout->addWidget(_textEdit);

        QObject::connect( _textEdit, SIGNAL(editingFinished()), this, SLOT(onTextChanged()) );
        // layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        _textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_textEdit, 0);

        if ( knob->isCustomHTMLText() ) {
            _textEdit->setReadOnlyNatron(true);
        }

        if (useRichText) {
            _richTextOptions = new QWidget(_container);
            _richTextOptionsLayout = new QHBoxLayout(_richTextOptions);
            _richTextOptionsLayout->setContentsMargins(0, 0, 0, 0);
            _richTextOptionsLayout->setSpacing(8);

            _fontCombo = new QFontComboBox(_richTextOptions);
            _fontCombo->setCurrentFont( makeFontFromState() );
            _fontCombo->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Font."), NATRON_NAMESPACE::WhiteSpaceNormal) );
            _richTextOptionsLayout->addWidget(_fontCombo);

            _fontSizeSpinBox = new SpinBox(_richTextOptions);
            _fontSizeSpinBox->setMinimum(1);
            _fontSizeSpinBox->setMaximum(100);
            _fontSizeSpinBox->setValue(knob->getFontSize());
            QObject::connect( _fontSizeSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onFontSizeChanged(double)) );
            _fontSizeSpinBox->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Font size."), NATRON_NAMESPACE::WhiteSpaceNormal) );
            _richTextOptionsLayout->addWidget(_fontSizeSpinBox);

            QPixmap pixBoldChecked, pixBoldUnchecked, pixItalicChecked, pixItalicUnchecked;
            appPTR->getIcon(NATRON_PIXMAP_BOLD_CHECKED, &pixBoldChecked);
            appPTR->getIcon(NATRON_PIXMAP_BOLD_UNCHECKED, &pixBoldUnchecked);
            appPTR->getIcon(NATRON_PIXMAP_ITALIC_CHECKED, &pixItalicChecked);
            appPTR->getIcon(NATRON_PIXMAP_ITALIC_UNCHECKED, &pixItalicUnchecked);
            QIcon boldIcon;
            boldIcon.addPixmap(pixBoldChecked, QIcon::Normal, QIcon::On);
            boldIcon.addPixmap(pixBoldUnchecked, QIcon::Normal, QIcon::Off);
            _setBoldButton = new Button(boldIcon, QString(), _richTextOptions);
            _setBoldButton->setIconSize( QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE) );
            _setBoldButton->setCheckable(true);
            _setBoldButton->setChecked(knob->getBoldActivated());
            _setBoldButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Bold."), NATRON_NAMESPACE::WhiteSpaceNormal) );
            _setBoldButton->setMaximumSize(18, 18);
            QObject::connect( _setBoldButton, SIGNAL(clicked(bool)), this, SLOT(boldChanged(bool)) );
            _richTextOptionsLayout->addWidget(_setBoldButton);

            QIcon italicIcon;
            italicIcon.addPixmap(pixItalicChecked, QIcon::Normal, QIcon::On);
            italicIcon.addPixmap(pixItalicUnchecked, QIcon::Normal, QIcon::Off);

            _setItalicButton = new Button(italicIcon, QString(), _richTextOptions);
            _setItalicButton->setCheckable(true);
            _setItalicButton->setChecked(knob->getItalicActivated());
            _setItalicButton->setIconSize( QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE) );
            _setItalicButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Italic."), NATRON_NAMESPACE::WhiteSpaceNormal) );
            _setItalicButton->setMaximumSize(18, 18);
            QObject::connect( _setItalicButton, SIGNAL(clicked(bool)), this, SLOT(italicChanged(bool)) );
            _richTextOptionsLayout->addWidget(_setItalicButton);

            QPixmap pixBlack(15, 15);
            pixBlack.fill(Qt::black);
            _fontColorButton = new Button(QIcon(pixBlack), QString(), _richTextOptions);
            _fontColorButton->setCheckable(false);
            _fontColorButton->setIconSize( QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE) );
            _fontColorButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Font color."), NATRON_NAMESPACE::WhiteSpaceNormal) );
            _fontColorButton->setMaximumSize(18, 18);
            QObject::connect( _fontColorButton, SIGNAL(clicked(bool)), this, SLOT(colorFontButtonClicked()) );
            _richTextOptionsLayout->addWidget(_fontColorButton);

            double r,g,b;
            knob->getFontColor(&r, &g, &b);
            QColor c;
            c.setRgbF(Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.));
            updateFontColorIcon(c);
            _richTextOptionsLayout->addStretch();

            _mainLayout->addWidget(_richTextOptions);


            ///Connect the slot after restoring
            QObject::connect( _fontCombo, SIGNAL(currentFontChanged(QFont)), this, SLOT(onCurrentFontChanged(QFont)) );
        }

        layout->addWidget(_container);
    } else if ( knob->isLabel() ) {
        const std::string& iconFilePath = knob->getIconLabel();
        if ( !iconFilePath.empty() ) {
            _label = new Label( layout->parentWidget() );

            if ( hasToolTip() ) {
                toolTip(_label);
            }
            layout->addWidget(_label);
        }
    } else {
        _lineEdit = new KnobLineEdit( shared_from_this(), 0, layout->parentWidget() );

        if ( hasToolTip() ) {
            toolTip(_lineEdit);
        }
        _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        layout->addWidget(_lineEdit);
        QObject::connect( _lineEdit, SIGNAL(editingFinished()), this, SLOT(onLineChanged()) );

        if ( knob->isCustomKnob() ) {
            _lineEdit->setReadOnly_NoFocusRect(true);
        }

        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_lineEdit, 0);
    }
} // createWidget

KnobGuiString::~KnobGuiString()
{
}

bool
KnobGuiString::parseFont(const QString & s, QFont* f, QColor* color)
{
    double r,g,b;
    QString family;
    bool bold;
    bool italic;
    int size;
    if (!KnobString::parseFont(s, &size, &family, &bold, &italic, &r, &g, &b)) {
        return false;
    }
    f->setFamily(family);
    f->setPointSize(size);
    f->setBold(bold);
    f->setItalic(italic);
    color->setRgbF(Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.));
    return true;
}

void
KnobGuiString::removeSpecificGui()
{
    if (_lineEdit) {
        _lineEdit->deleteLater();
    }
    if (_container) {
        _container->deleteLater();
    }
}

std::string
KnobGuiString::getDescriptionLabel() const
{
    KnobStringPtr k = _knob.lock();
    bool isLabel = k->isLabel();

    if (isLabel) {
        return k->getValue();
    } else {
        return k->getLabel();
    }
}

void
KnobGuiString::onLineChanged()
{
    if ( !_lineEdit->isEnabled() || _lineEdit->isReadOnly() ) {
        return;
    }
    std::string oldText = _knob.lock()->getValue(0);
    std::string newText = _lineEdit->text().toStdString();

    if (oldText != newText) {
        pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldText, newText) );
    }

}

void
KnobGuiString::onTextChanged()
{
    QString txt = _textEdit->toPlainText();
    pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), _knob.lock()->getValue(0), txt.toStdString() ) );
}


void
KnobGuiString::updateFontColorIcon(const QColor & color)
{
    QPixmap p(18, 18);

    p.fill(color);
    _fontColorButton->setIcon( QIcon(p) );
}

void
KnobGuiString::onCurrentFontChanged(const QFont & font)
{
    KnobStringPtr knob = _knob.lock();
    knob->setFontFamily(font.family().toStdString());
    updateGUI(0);
    Q_EMIT fontPropertyChanged();
}



void
KnobGuiString::onFontSizeChanged(double size)
{
    KnobStringPtr knob = _knob.lock();
    knob->setFontSize(size);
    updateGUI(0);
    Q_EMIT fontPropertyChanged();
}

void
KnobGuiString::boldChanged(bool toggled)
{
    KnobStringPtr knob = _knob.lock();
    knob->setBoldActivated(toggled);
    updateGUI(0);
    Q_EMIT fontPropertyChanged();
}

void
KnobGuiString::mergeFormat(const QTextCharFormat & fmt)
{
    QTextCursor cursor = _textEdit->textCursor();

    if ( cursor.hasSelection() ) {
        cursor.mergeCharFormat(fmt);
        _textEdit->mergeCurrentCharFormat(fmt);
    }
}

void
KnobGuiString::colorFontButtonClicked()
{


    QColorDialog dialog(_textEdit);
    dialog.setOption(QColorDialog::DontUseNativeDialog);
    QObject::connect( &dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(updateFontColorIcon(QColor)) );

    KnobStringPtr knob = _knob.lock();

    QColor currentColor;
    {
        double r,g,b;
        knob->getFontColor(&r, &g, &b);
        currentColor.setRgbF(Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.));
    }
    dialog.setCurrentColor(currentColor);
    if ( dialog.exec() ) {
        currentColor = dialog.currentColor();
        double r = currentColor.redF();
        double g = currentColor.greenF();
        double b = currentColor.blueF();
        knob->setFontColor(r, g, b);
        updateGUI(0);
        Q_EMIT fontPropertyChanged();

    }
    updateFontColorIcon(currentColor);
}


void
KnobGuiString::italicChanged(bool toggled)
{
    KnobStringPtr knob = _knob.lock();
    knob->setItalicActivated(toggled);
    updateGUI(0);
    Q_EMIT fontPropertyChanged();
}



void
KnobGuiString::updateGUI(int /*dimension*/)
{
    KnobStringPtr knob = _knob.lock();
    std::string value = knob->getValue(0);

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->blockSignals(true);
        QTextCursor cursor = _textEdit->textCursor();
        int pos = cursor.position();
        int selectionStart = cursor.selectionStart();
        int selectionEnd = cursor.selectionEnd();

        QString txt = QString::fromUtf8( value.c_str() );

        if ( knob->isCustomHTMLText() ) {
            _textEdit->setHtml(txt);
        } else {
            //QString text = knob->decorateStringWithCurrentState(txt);
            _textEdit->setPlainText(txt);
        }

        if ( pos < txt.size() ) {
            cursor.setPosition(pos);
        } else {
            cursor.movePosition(QTextCursor::End);
        }

        ///restore selection
        cursor.setPosition(selectionStart);
        cursor.setPosition(selectionEnd, QTextCursor::KeepAnchor);

        _textEdit->setTextCursor(cursor);
        _textEdit->blockSignals(false);
    } else if ( knob->isLabel() ) {
        onLabelChanged();

        // If the knob has a label as an icon, set the content of the knob into a label
        const std::string& iconFilePath = knob->getIconLabel();
        if ( _label && !iconFilePath.empty() ) {
            QString txt = QString::fromUtf8( knob->getValue().c_str() );
            txt.replace( QLatin1String("\n"), QLatin1String("<br>") );
            _label->setText(txt);
        }
    } else {
        assert(_lineEdit);
        _lineEdit->setText( QString::fromUtf8( value.c_str() ) );
    }
} // KnobGuiString::updateGUI

void
KnobGuiString::_hide()
{
    KnobStringPtr knob = _knob.lock();

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->hide();
    } else if ( knob->isLabel() ) {
        if (_label) {
            _label->hide();
        }
    } else {
        assert(_lineEdit);
        _lineEdit->hide();
    }
}

void
KnobGuiString::_show()
{
    KnobStringPtr knob = _knob.lock();

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->show();
    } else if ( knob->isLabel() ) {
        if (_label) {
            _label->show();
        }
    } else {
        assert(_lineEdit);
        _lineEdit->show();
    }
}

void
KnobGuiString::setEnabled()
{
    KnobStringPtr knob = _knob.lock();
    bool b = knob->isEnabled(0)  &&  knob->getExpression(0).empty();

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        //_textEdit->setEnabled(b);
        //_textEdit->setReadOnly(!b);
        if ( !knob->isCustomHTMLText() ) {
            _textEdit->setReadOnlyNatron(!b);
        }
    } else if ( knob->isLabel() ) {
        if (_label) {
            _label->setEnabled(b);
        }
    } else {
        assert(_lineEdit);
        //_lineEdit->setEnabled(b);
        if ( !knob->isCustomKnob() ) {
            _lineEdit->setReadOnly(!b);
        }
    }
}

void
KnobGuiString::reflectAnimationLevel(int /*dimension*/,
                                     AnimationLevelEnum level)
{
    KnobStringPtr knob = _knob.lock();
    int value;

    switch (level) {
    case eAnimationLevelNone:
        value = 0;
        break;
    case eAnimationLevelInterpolatedValue:
        value = 1;
        break;
    case eAnimationLevelOnKeyframe:
        value = 2;

        break;
    default:
        value = 0;
        break;
    }

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->setAnimation(value);
    } else if ( knob->isLabel() ) {
        //assert(_label);
    } else {
        assert(_lineEdit);
        _lineEdit->setAnimation(value);
    }
}

void
KnobGuiString::setReadOnly(bool readOnly,
                           int /*dimension*/)
{
    if (_textEdit) {
        if ( !_knob.lock()->isCustomHTMLText() ) {
            _textEdit->setReadOnlyNatron(readOnly);
        }
    } else if (_lineEdit) {
        if ( !_knob.lock()->isCustomKnob() ) {
            _lineEdit->setReadOnly_NoFocusRect(readOnly);
        }
    }
}

void
KnobGuiString::setDirty(bool dirty)
{
    if (_textEdit) {
        _textEdit->setDirty(dirty);
    } else if (_lineEdit) {
        _lineEdit->setDirty(dirty);
    }
}

KnobIPtr
KnobGuiString::getKnob() const
{
    return _knob.lock();
}

void
KnobGuiString::reflectExpressionState(int /*dimension*/,
                                      bool hasExpr)
{
    bool isEnabled = _knob.lock()->isEnabled(0);

    if (_textEdit) {
        _textEdit->setAnimation(3);
        if ( !_knob.lock()->isCustomHTMLText() ) {
            _textEdit->setReadOnlyNatron(hasExpr || !isEnabled);
        }
    } else if (_lineEdit) {
        _lineEdit->setAnimation(3);
        _lineEdit->setReadOnly_NoFocusRect(hasExpr || !isEnabled);
    }
}

void
KnobGuiString::updateToolTip()
{
    if ( hasToolTip() ) {

        if (_textEdit) {
            bool useRichText = _knob.lock()->usesRichText();
            QString tt = toolTip(0);
            if (useRichText) {
                tt += tr("This text area supports html encoding. "
                         "Please check <a href=http://qt-project.org/doc/qt-5/richtext-html-subset.html>Qt website</a> for more info.");
            }
            QKeySequence seq(Qt::CTRL + Qt::Key_Return);
            tt += tr("Use %1 to validate changes made to the text.").arg( seq.toString(QKeySequence::NativeText) );
            _textEdit->setToolTip(tt);
        } else if (_lineEdit) {
            toolTip(_lineEdit);
        } else if (_label) {
            toolTip(_label);
        }
    }
}

void
KnobGuiString::reflectModificationsState()
{
    bool hasModif = _knob.lock()->hasModifications();

    if (_lineEdit) {
        _lineEdit->setAltered(!hasModif);
    }
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_KnobGuiString.cpp"
