#ifndef STATS_OVERLAY_H
#define STATS_OVERLAY_H

#include "qwt_widget_overlay.h"
#include <QMap>
#include <QString>
#include <QPointF>

class PlotWidget;
class QwtPlotCurve;

/**
 * @brief The StatsOverlay class provides a Grafana-style metrics display overlay
 * 
 * This overlay displays current curve values in a large, prominent format similar
 * to Grafana dashboard panels. Values update based on tracker position or show
 * the latest values when no tracker is active. The visualization uses a semi-transparent
 * background with curve colors for values.
 */
class StatsOverlay : public QwtWidgetOverlay
{
public:
    /**
     * @brief Constructs a Grafana-style overlay for the given plot
     * 
     * @param parent The parent widget (typically the canvas)
     * @param plotWidget The PlotWidget that this overlay is associated with
     */
    explicit StatsOverlay(QWidget* parent, PlotWidget* plotWidget);
    
    /**
     * @brief Updates the curve values displayed in the overlay
     * 
     * Refreshes all current values from the underlying curves.
     */
    void updateData();
    
    /**
     * @brief Updates displayed values based on the tracker position
     * 
     * @param pos The current tracker position (x,y)
     */
    void updateTrackerPosition(const QPointF& pos);
    
protected:
    /**
     * @brief Draws the Grafana-style metrics overlay
     * 
     * @param painter The painter to use for drawing
     */
    virtual void drawOverlay(QPainter* painter) const override;
    
    /**
     * @brief Provides a hint for the mask region
     * 
     * @return The region that should be used for the overlay mask
     */
    virtual QRegion maskHint() const override;
    
private:
    /**
     * @brief Finds the value in a curve closest to the given x position
     * 
     * @param curve The curve to search
     * @param x_pos The x position
     * @return The corresponding y value
     */
    double getCurrentValue(const QwtPlotCurve* curve, double x_pos) const;
    
    PlotWidget* _plot_widget;
    QMap<QString, double> _current_values;
    QPointF _tracker_pos;
};

#endif // STATS_OVERLAY_H
