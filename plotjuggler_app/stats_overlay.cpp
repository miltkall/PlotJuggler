#include "stats_overlay.h"
#include "plotwidget.h"
#include "qwt_plot_curve.h"
#include "qwt_series_data.h"
#include "qwt_text.h"
#include <QPainter>
#include <algorithm>
#include <limits>
// #include <QDebug>

StatsOverlay::StatsOverlay(QWidget* parent, PlotWidget* plotWidget) :
    QwtWidgetOverlay(parent), _plot_widget(plotWidget)
{
    setMaskMode(QwtWidgetOverlay::MaskHint);
    setRenderMode(QwtWidgetOverlay::AutoRenderMode);
    updateData();
}

void StatsOverlay::drawOverlay(QPainter* painter) const
{
    if (!_plot_widget || _plot_widget->curveList().empty()) {
        return;
    }
    
    // Constants for Grafana-like appearance
    const int NAME_FONT_SIZE = 16;
    const int VALUE_FONT_SIZE = 48;
    const int NAME_VALUE_SPACING = 5;
    const int ITEM_SPACING = 30;
    const QColor BACKGROUND_COLOR(20, 20, 20, 200);
    
    // Draw background
    QRect canvasRect = parentWidget()->rect();
    painter->setPen(Qt::NoPen);
    painter->setBrush(BACKGROUND_COLOR);
    painter->drawRect(canvasRect);
    
    // Collect all visible curves
    struct CurveInfo {
        QString name;
        double value;
        QColor color;
    };
    
    QVector<CurveInfo> visibleCurves;
    
    for (const auto& it : _plot_widget->curveList()) {
        if (it.curve->isVisible()) {
            QString title = it.curve->title().text();
            QColor color = it.curve->pen().color();
            
            double value = 0.0;
            auto data_it = _current_values.find(title);
            if (data_it != _current_values.end()) {
                value = data_it.value();
            }
            
            visibleCurves.append({title, value, color});
        }
    }
    
    if (visibleCurves.isEmpty()) {
        return;
    }
    
    // Set up fonts and metrics
    QFont nameFont = painter->font();
    nameFont.setPointSize(NAME_FONT_SIZE);
    
    QFont valueFont = painter->font();
    valueFont.setPointSize(VALUE_FONT_SIZE);
    valueFont.setBold(true);
    
    // Calculate metrics for spacing
    QFontMetrics nameFM(nameFont);
    QFontMetrics valueFM(valueFont);
    
    // Calculate total height needed for all curves
    int nameHeight = nameFM.height();
    int valueHeight = valueFM.height();
    int itemHeight = nameHeight + valueHeight + NAME_VALUE_SPACING;
    
    int totalHeight = visibleCurves.size() * itemHeight + 
                    (visibleCurves.size() - 1) * ITEM_SPACING;
    
    // Center vertically in canvas
    int startY = (canvasRect.height() - totalHeight) / 2;
    
    // Draw each curve's metrics
    for (int i = 0; i < visibleCurves.size(); i++) {
        const auto& curve = visibleCurves[i];
        
        // Calculate positions - each "item" consists of name + value
        int itemY = startY + i * (itemHeight + ITEM_SPACING);
        
        // Draw name
        int nameY = itemY;
        painter->setFont(nameFont);
        painter->setPen(Qt::white);
        painter->drawText(QRect(0, nameY, canvasRect.width(), nameHeight),
                        Qt::AlignHCenter, curve.name);
        
        // Draw value - positioned below the name
        int valueY = nameY + nameHeight + NAME_VALUE_SPACING;
        painter->setFont(valueFont);
        painter->setPen(curve.color);
        QString valueText = QString::number(curve.value, 'f', 2);
        painter->drawText(QRect(0, valueY, canvasRect.width(), valueHeight),
                        Qt::AlignHCenter, valueText);
    }
}

QRegion StatsOverlay::maskHint() const
{
    return parentWidget()->rect();
}

void StatsOverlay::updateData()
{
    if (!_plot_widget) {
        return;
    }
    
    // Clear previous data
    _current_values.clear();
    
    int visible_curves = 0;
    
    // Get latest values for all visible curves
    for (const auto& it : _plot_widget->curveList()) {
        if (!it.curve->isVisible() || !it.curve->data() || it.curve->dataSize() == 0) {
            continue;
        }
        
        visible_curves++;
        const QString title = it.curve->title().text();
        
        // Use the last value as the current value initially
        size_t last_idx = it.curve->dataSize() - 1;
        double value = it.curve->data()->sample(last_idx).y();
        
        _current_values[title] = value;
    }
    
    // qDebug() << "StatsOverlay::updateData - Updated values for" << visible_curves << "visible curves";
    
    // Update with current tracker position if available
    if (!_tracker_pos.isNull()) {
        updateTrackerPosition(_tracker_pos);
    }
    
    update();
}

void StatsOverlay::updateTrackerPosition(const QPointF& pos)
{
    _tracker_pos = pos;
    double x_pos = pos.x();
    
    for (const auto& it : _plot_widget->curveList()) {
        if (!it.curve->isVisible() || !it.curve->data() || it.curve->dataSize() == 0) {
            continue;
        }
        
        const QString title = it.curve->title().text();
        double value = getCurrentValue(it.curve, x_pos);
        _current_values[title] = value;
    }
    
    update();
}

double StatsOverlay::getCurrentValue(const QwtPlotCurve* curve, double x_pos) const
{
    if (!curve || curve->dataSize() == 0) {
        return 0.0;
    }
    
    // Use QwtTimeseries interface when available
    if (auto series = dynamic_cast<const QwtTimeseries*>(curve->data()))
    {
        // Need const_cast because sampleFromTime isn't marked const
        auto pointXY = const_cast<QwtTimeseries*>(series)->sampleFromTime(x_pos);
        if (pointXY)
        {
            return pointXY.value().y();
        }
    }
    
    // Just use the standard QwtPlotCurve method to find the closest point
    return curve->sample(curve->closestPoint(QPointF(x_pos, 0))).y();
}
