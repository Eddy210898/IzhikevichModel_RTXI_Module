/*
 * Copyright (C) 2011 Georgia Institute of Technology, University of Utah,
 * Weill Cornell Medical College
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This is a template implementation file for a user module derived from
 * DefaultGUIModel with a custom GUI.
 */

#include "IzhikevichModelPlugin.h"
#include <iostream>
#include <main_window.h>

extern "C" Plugin::Object *
createRTXIPlugin(void)
{
  return new IzhikevichModelPlugin();
}

static DefaultGUIModel::variable_t vars[] = {
    {
        "I",
        "",
        DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
    },

    {
        "a",
        "",
        DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
    },
    {
        "b",
        "",
        DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
    },
    {
        "c",
        "",
        DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
    },
    {
        "d",
        "",
        DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
    },

    {
        "v0",
        "",
        DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
    },
    {
        "u0",
        "",
        DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
    },

    {
        "Vm (mV)",
        "Membrane potential (in mV)",
        DefaultGUIModel::OUTPUT,
    },

    {
        "Isyn (nA)",
        "Synaptic input current (in nA)",
        DefaultGUIModel::INPUT,
    },
    {
        "v",
        "",
        DefaultGUIModel::STATE,
    },
    {
        "dt",
        "",
        DefaultGUIModel::STATE,
    },
    {
        "syn",
        "",
        DefaultGUIModel::STATE,
    },
};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

IzhikevichModelPlugin::IzhikevichModelPlugin(void)
    : DefaultGUIModel("Izhikevich Model Plugin with Custom GUI", ::vars, ::num_vars)
{
  setWhatsThis("<p><b>IzhikevichModelPlugin:</b><br>QWhatsThis description.</p>");
  DefaultGUIModel::createGUI(vars,
                             num_vars); // this is required to create the GUI
  customizeGUI();
  initParameters();
  update(INIT); // this is optional, you may place initialization code directly
                // into the constructor
  refresh();    // this is required to update the GUI with parameter and state
                // values
  QTimer::singleShot(0, this, SLOT(resizeMe()));
}

IzhikevichModelPlugin::~IzhikevichModelPlugin(void)
{
}

double IzhikevichModelPlugin::getXValue(int kValue, double x, double dX)
{
  if (kValue == 1)
  {
    return x;
  }
  else if (kValue == 2 || kValue == 3)
  {
    return x + (0.5 * dX);
  }
  else if (kValue == 4)
  {
    return x + dX;
  }
  else
  {
    throw "K value not recognize";
  }
}

double IzhikevichModelPlugin::getYValue(int kValue, double y, double dX, double k /*= 0*/)
{
  if (kValue == 1)
  {
    return y;
  }
  else if (kValue == 2 || kValue == 3)
  {
    return y + (0.5 * k * dX);
  }
  else if (kValue == 4)
  {
    return y + (k * dX);
  }
  else
  {
    throw "K value not recognize";
  }
}

double IzhikevichModelPlugin::getNextRungeKuta(double Xo, double Yo, double dX, double F(double, double, double[]), double args[])
{
  double x = getXValue(1, Xo, dX);
  double y = getYValue(1, Yo, dX);
  double kA = F(x, y, args);
  x = getXValue(2, Xo, dX);
  y = getYValue(2, Yo, dX, kA);
  double kB = F(x, y, args);
  x = getXValue(3, Xo, dX);
  y = getYValue(3, Yo, dX, kA);
  double kC = F(x, y, args);
  x = getXValue(4, Xo, dX);
  y = getYValue(4, Yo, dX, kA);
  double kD = F(x, y, args);
  double kT = kA + kB + kC + kD;
  double newValToAdd = (dX * kT) / 6;
  return Yo + newValToAdd;
}

void IzhikevichModelPlugin::izhikevichStep(double vO, double uO, double t, double dT, double I, double a, double b, double c, double d)
{
  if (vO >= 30)
  {
    vO = c;
    uO = uO + d;
    v = vO;
    u = uO;
    return;
  }
  double arg[] = {
      a,  //0
      b,  //1
      c,  //2
      d,  //3
      vO, //4
      uO, //5
      I   //6

  };
  double vF = getNextRungeKuta(
      t, vO, dT, [](double t, double v, double args[]) {
        double fE = 0.04 * v * v;
        double sE = 5 * v;
        return fE + sE + 140 - args[5] + args[6];
      },
      arg);

  double uF = getNextRungeKuta(
      t, uO, dT, [](double t, double u, double args[]) {
        double fE = args[1] * args[4];
        double sE = fE - u;
        return args[0] * sE;
      },
      arg);
  v = vF;
  u = uF;
  return;
}

void IzhikevichModelPlugin::execute(void)
{
  izhikevichStep(v, u, period, dt, I, a, b, c, d);
  output(0) = v;
  return;
}

void IzhikevichModelPlugin::initParameters(void)
{
  a = 0.02;
  b = 0.2;
  c = -65;
  d = 2;
  v = -65;
  u = v * b;
  dt = 0.1;
}

void IzhikevichModelPlugin::update(DefaultGUIModel::update_flags_t flag)
{
  switch (flag)
  {
  case INIT:
    period = RT::System::getInstance()->getPeriod() * 1e-6; // ms

    setParameter("I", I);

    setParameter("a", a);
    setParameter("b", b);
    setParameter("c", c);
    setParameter("d", d);

    setParameter("v0", v);
    setParameter("u0", u);

    setState("v", v);
    setState("dt", dt);
    break;

  case MODIFY:

    a = getParameter("a").toDouble();
    b = getParameter("b").toDouble();
    c = getParameter("c").toDouble();
    d = getParameter("d").toDouble();

    I = getParameter("I").toDouble();

    v = getParameter("v0").toDouble();
    u = getParameter("u0").toDouble();
    break;

  case UNPAUSE:

    break;

  case PAUSE:
    break;

  case PERIOD:
    period = RT::System::getInstance()->getPeriod() * 1e-6; // ms

    break;

  default:
    break;
  }
}

void IzhikevichModelPlugin::customizeGUI(void)
{
  /*
  QGridLayout *customlayout = DefaultGUIModel::getLayout();

  QGroupBox *button_group = new QGroupBox;

  QPushButton *abutton = new QPushButton("Button A");
  QPushButton *bbutton = new QPushButton("Button B");
  QHBoxLayout *button_layout = new QHBoxLayout;
  button_group->setLayout(button_layout);
  button_layout->addWidget(abutton);
  button_layout->addWidget(bbutton);
  QObject::connect(abutton, SIGNAL(clicked()), this, SLOT(aBttn_event()));
  QObject::connect(bbutton, SIGNAL(clicked()), this, SLOT(bBttn_event()));

  customlayout->addWidget(button_group, 0, 0);
  setLayout(customlayout);
  */
}
