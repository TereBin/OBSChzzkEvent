#include "chzzk-alert-status-dock.hpp"
#include "chzzk-alert-connection.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QFormLayout>
#include <QLabel>
#include <QPointer>
#include <QTimer>
#include <QWidget>

namespace {
constexpr auto kDockId = "obs-chzzk-alerts-status";
QPointer<QWidget> g_widget;
bool g_exiting = false;

QString text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}
QString timeText(qint64 value)
{
	return value ? QDateTime::fromMSecsSinceEpoch(value).toString("MM-dd HH:mm:ss") : text("Status.None");
}

// Child labels, timer and callback context share the dock widget's lifetime.
class AlertStatusWidget final : public QWidget {
public:
	AlertStatusWidget()
	{
		auto *layout = new QFormLayout(this);
		auto field = [&](const char *key) {
			auto *label = new QLabel(this);
			label->setTextFormat(Qt::PlainText);
			label->setWordWrap(true);
			layout->addRow(text(key), label);
			return label;
		};
		server_ = field("Status.Server");
		bridge_ = field("Status.Bridge");
		contact_ = field("Status.Contact");
		follow_ = field("Status.Follow");
		pending_ = field("Status.Pending");
		error_ = field("Status.Error");
		refresh();
		auto *timer = new QTimer(this);
		QObject::connect(timer, &QTimer::timeout, this, [this] { refresh(); });
		timer->start(1000);
	}

private:
	void refresh()
	{
		const auto state = alertConnectionState().snapshot();
		server_->setText(text(state.listening ? "Status.Running" : "Status.Stopped"));
		bridge_->setText(state.bridges ? text("Status.Connected").arg(state.bridges) : text("Status.Waiting"));
		contact_->setText(timeText(state.lastContact));
		follow_->setText(timeText(state.lastFollow));
		pending_->setText(QString::number(state.pending));
		const auto key = state.lastError.isEmpty() ? QByteArray("Status.None")
							   : QByteArray("Status.Error.") + state.lastError.toUtf8();
		error_->setText(text(key.constData()));
	}
	QLabel *server_;
	QLabel *bridge_;
	QLabel *contact_;
	QLabel *follow_;
	QLabel *pending_;
	QLabel *error_;
};

void createDock()
{
	if (g_widget)
		return;
	auto *widget = new AlertStatusWidget;
	if (!obs_frontend_add_dock_by_id(kDockId, obs_module_text("Status.Title"), widget)) {
		delete widget;
		return;
	}
	g_widget = widget;
}

void frontendEvent(obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING)
		createDock();
	else if (event == OBS_FRONTEND_EVENT_EXIT) {
		g_exiting = true;
		obs_frontend_remove_event_callback(frontendEvent, nullptr);
		// Destroy timer callbacks before module unload; the frontend owns the dock shell.
		delete g_widget.data();
	}
}
} // namespace

void registerAlertStatusDock()
{
	g_exiting = false;
	obs_frontend_add_event_callback(frontendEvent, nullptr);
}

void unregisterAlertStatusDock()
{
	if (!g_exiting) {
		obs_frontend_remove_event_callback(frontendEvent, nullptr);
		if (g_widget)
			obs_frontend_remove_dock(kDockId);
	}
}
