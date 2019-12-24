#include "qblcanvas.h"

#include <stdlib.h>
#include <vector>

// ============================================================================
// [MainWindow]
// ============================================================================

static double randomDouble() noexcept {
  return double(rand() % 65535) * (1.0 / 65535.0);
}

static double randomSign() noexcept {
  return (rand() % 65535) >= 32768 ? 1.0 : -1.0;
}

static BLRgba32 randomColor() noexcept {
  unsigned r = (rand() % 255);
  unsigned g = (rand() % 255);
  unsigned b = (rand() % 255);
  unsigned a = (rand() % 255);
  return BLRgba32(r, g, b, a);
}

static QPainter::CompositionMode Blend2DCompOpToQtCompositionMode(uint32_t compOp) {
  switch (compOp) {
    default:
    case BL_COMP_OP_SRC_OVER   : return QPainter::CompositionMode_SourceOver;
    case BL_COMP_OP_SRC_COPY   : return QPainter::CompositionMode_Source;
    case BL_COMP_OP_SRC_IN     : return QPainter::CompositionMode_SourceIn;
    case BL_COMP_OP_SRC_OUT    : return QPainter::CompositionMode_SourceOut;
    case BL_COMP_OP_SRC_ATOP   : return QPainter::CompositionMode_SourceAtop;
    case BL_COMP_OP_DST_OVER   : return QPainter::CompositionMode_DestinationOver;
    case BL_COMP_OP_DST_COPY   : return QPainter::CompositionMode_Destination;
    case BL_COMP_OP_DST_IN     : return QPainter::CompositionMode_DestinationIn;
    case BL_COMP_OP_DST_OUT    : return QPainter::CompositionMode_DestinationOut;
    case BL_COMP_OP_DST_ATOP   : return QPainter::CompositionMode_DestinationAtop;
    case BL_COMP_OP_XOR        : return QPainter::CompositionMode_Xor;
    case BL_COMP_OP_CLEAR      : return QPainter::CompositionMode_Clear;
    case BL_COMP_OP_PLUS       : return QPainter::CompositionMode_Plus;
    case BL_COMP_OP_MULTIPLY   : return QPainter::CompositionMode_Multiply;
    case BL_COMP_OP_SCREEN     : return QPainter::CompositionMode_Screen;
    case BL_COMP_OP_OVERLAY    : return QPainter::CompositionMode_Overlay;
    case BL_COMP_OP_DARKEN     : return QPainter::CompositionMode_Darken;
    case BL_COMP_OP_LIGHTEN    : return QPainter::CompositionMode_Lighten;
    case BL_COMP_OP_COLOR_DODGE: return QPainter::CompositionMode_ColorDodge;
    case BL_COMP_OP_COLOR_BURN : return QPainter::CompositionMode_ColorBurn;
    case BL_COMP_OP_HARD_LIGHT : return QPainter::CompositionMode_HardLight;
    case BL_COMP_OP_SOFT_LIGHT : return QPainter::CompositionMode_SoftLight;
    case BL_COMP_OP_DIFFERENCE : return QPainter::CompositionMode_Difference;
    case BL_COMP_OP_EXCLUSION  : return QPainter::CompositionMode_Exclusion;
  }
}
class MainWindow : public QWidget {
  Q_OBJECT

public:
  QTimer _timer;
  QSlider _slider;
  QBLCanvas _canvas;
  QComboBox _rendererSelect;
  QComboBox _compOpSelect;
  QCheckBox _limitFpsCheck;

  std::vector<BLPoint> _coords;
  std::vector<BLPoint> _steps;
  std::vector<BLRgba32> _colors;
  uint32_t _compOp;
  double _rectSize;

  MainWindow()
    : _compOp(BL_COMP_OP_SRC_OVER),
      _rectSize(64.0) {

    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setSpacing(5);

    _rendererSelect.addItem("Blend2D", QVariant(int(QBLCanvas::RendererB2D)));
    _rendererSelect.addItem("Qt", QVariant(int(QBLCanvas::RendererQt)));
    _compOpSelect.addItem("SrcOver", QVariant(int(BL_COMP_OP_SRC_OVER)));
    _compOpSelect.addItem("SrcCopy", QVariant(int(BL_COMP_OP_SRC_COPY)));
    _compOpSelect.addItem("Plus", QVariant(int(BL_COMP_OP_PLUS)));
    _compOpSelect.addItem("Screen", QVariant(int(BL_COMP_OP_SCREEN)));
    _compOpSelect.addItem("Overlay", QVariant(int(BL_COMP_OP_OVERLAY)));
    _compOpSelect.addItem("Lighten", QVariant(int(BL_COMP_OP_LIGHTEN)));
    _compOpSelect.addItem("Difference", QVariant(int(BL_COMP_OP_DIFFERENCE)));
    _limitFpsCheck.setText(QLatin1Literal("Limit FPS"));

    _slider.setOrientation(Qt::Horizontal);
    _slider.setMinimum(1);
    _slider.setMaximum(5000);
    _slider.setSliderPosition(50);

    _canvas.onRenderB2D = std::bind(&MainWindow::onRenderB2D, this, std::placeholders::_1);
    _canvas.onRenderQt = std::bind(&MainWindow::onRenderQt, this, std::placeholders::_1);

    connect(&_rendererSelect, SIGNAL(activated(int)), SLOT(onRendererChanged(int)));
    connect(&_compOpSelect, SIGNAL(activated(int)), SLOT(onCompOpChanged(int)));
    connect(&_limitFpsCheck, SIGNAL(stateChanged(int)), SLOT(onLimitFpsChanged(int)));
    connect(&_slider, SIGNAL(valueChanged(int)), SLOT(onRectsSizeChanged(int)));

    grid->addWidget(new QLabel("Renderer:"), 0, 0);
    grid->addWidget(&_rendererSelect, 0, 1);
    grid->addWidget(new QLabel("Comp Op:"), 0, 2);
    grid->addWidget(&_compOpSelect, 0, 3);
    grid->addWidget(&_limitFpsCheck, 0, 4);
    grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, 5);
    grid->addWidget(&_slider, 1, 0, 1, 6);

    vBox->addLayout(grid);
    vBox->addWidget(&_canvas);
    setLayout(vBox);

    connect(&_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    _timer.setInterval(2);

    onInit();
    _updateTitle();
  }

  void showEvent(QShowEvent* event) override { _timer.start(); }
  void hideEvent(QHideEvent* event) override { _timer.stop(); }
  void keyPressEvent(QKeyEvent* event) override {}

  void onInit() {
    setRectsSize(_slider.sliderPosition());
  }

  Q_SLOT void onRendererChanged(int index) { _canvas.setRendererType(_rendererSelect.itemData(index).toInt());  }
  Q_SLOT void onCompOpChanged(int index) { _compOp = _compOpSelect.itemData(index).toInt(); };
  Q_SLOT void onLimitFpsChanged(int value) { _timer.setInterval(value ? 1000 / 60 : 2); }
  Q_SLOT void onRectsSizeChanged(int value) { setRectsSize(size_t(value)); }

  Q_SLOT void onTimer() {
    double w = _canvas.blImage.width();
    double h = _canvas.blImage.height();

    size_t size = _coords.size();
    for (size_t i = 0; i < size; i++) {
      BLPoint& vertex = _coords[i];
      BLPoint& step = _steps[i];

      vertex += step;
      if (vertex.x <= 0.0 || vertex.x >= w) {
        step.x = -step.x;
        vertex.x = blMin(vertex.x + step.x, w);
      }

      if (vertex.y <= 0.0 || vertex.y >= h) {
        step.y = -step.y;
        vertex.y = blMin(vertex.y + step.y, h);
      }
    }

    _canvas.updateCanvas(true);
    _updateTitle();
  }

  void onRenderB2D(BLContext& ctx) noexcept {
    ctx.setFillStyle(BLRgba32(0xFF000000));
    ctx.fillAll();

    size_t size = _coords.size();
    double halfSize = _rectSize * 0.5;

    ctx.setCompOp(_compOp);

    for (size_t i = 0; i < size; i++) {
      ctx.setFillStyle(_colors[i]);
      ctx.fillBox(_coords[i].x - halfSize, _coords[i].y - halfSize, _coords[i].x + halfSize, _coords[i].y + halfSize);
    }
  }

  void onRenderQt(QPainter& ctx) noexcept {
    ctx.fillRect(0, 0, _canvas.width(), _canvas.height(), QColor(0, 0, 0));

    ctx.setRenderHint(QPainter::Antialiasing, true);
    ctx.setBrush(QColor(255, 255, 255));

    // ctx.setOpacity(0.5);
    ctx.setCompositionMode(Blend2DCompOpToQtCompositionMode(_compOp));

    size_t size = _coords.size();
    double halfSize = _rectSize * 0.5;

    for (size_t i = 0; i < size; i++) {
      ctx.fillRect(QRectF(_coords[i].x - halfSize, _coords[i].y - halfSize, _rectSize, _rectSize),
                   QColor(_colors[i].r, _colors[i].g, _colors[i].b, _colors[i].a));
    }
  }

  void setRectsSize(size_t size) {
    double w = _canvas.blImage.width();
    double h = _canvas.blImage.height();
    size_t i = _coords.size();

    _coords.resize(size);
    _steps.resize(size);
    _colors.resize(size);

    while (i < size) {
      _coords[i].reset(randomDouble() * w,
                       randomDouble() * h);
      _steps[i].reset((randomDouble() * 0.5 + 0.05) * randomSign(),
                      (randomDouble() * 0.5 + 0.05) * randomSign());
      _colors[i].reset(randomColor());
      i++;
    }
  }

  void _updateTitle() {
    char buf[256];
    snprintf(buf, 256, "Rectangles Sample [%dx%d] [N=%zu] [%.1f FPS]",
      _canvas.width(),
      _canvas.height(),
      _coords.size(),
      _canvas.fps());

    QString title = QString::fromUtf8(buf);
    if (title != windowTitle())
      setWindowTitle(title);
  }
};

// ============================================================================
// [Main]
// ============================================================================

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow win;

  win.setMinimumSize(QSize(400, 320));
  win.resize(QSize(580, 520));
  win.show();

  return app.exec();
}

#include "bl-qt-rects.moc"
