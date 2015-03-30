/*
 * (C) Copyright 2015 Kurento (http://kurento.org/)
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "kmssdpagent.h"
#include "sdp_utils.h"
#include "kmssdprtpavpfmediahandler.h"

#define OBJECT_NAME "rtpavpfmediahandler"

GST_DEBUG_CATEGORY_STATIC (kms_sdp_rtp_avpf_media_handler_debug_category);
#define GST_CAT_DEFAULT kms_sdp_rtp_avpf_media_handler_debug_category

#define parent_class kms_sdp_rtp_avpf_media_handler_parent_class

G_DEFINE_TYPE_WITH_CODE (KmsSdpRtpAvpfMediaHandler,
    kms_sdp_rtp_avpf_media_handler, KMS_TYPE_SDP_RTP_AVP_MEDIA_HANDLER,
    GST_DEBUG_CATEGORY_INIT (kms_sdp_rtp_avpf_media_handler_debug_category,
        OBJECT_NAME, 0, "debug category for sdp rtp avpf media_handler"));

#define SDP_MEDIA_RTP_AVPF_PROTO "RTP/AVPF"
#define SDP_MEDIA_RTP_AVPF_DEFAULT_NACK TRUE
#define SDP_MEDIA_RTCP_FB "rtcp-fb"
#define SDP_MEDIA_RTCP_FB_NACK "nack"
#define SDP_MEDIA_RTCP_FB_CCM "ccm"
#define SDP_MEDIA_RTCP_FB_GOOG_REMB "goog-remb"
#define SDP_MEDIA_RTCP_FB_PLI "pli"
#define SDP_MEDIA_RTCP_FB_FIR "fir"

static gchar *video_rtcp_fb_enc[] = {
  "VP8",
  "H264"
};

#define KMS_SDP_RTP_AVPF_MEDIA_HANDLER_GET_PRIVATE(obj) (  \
  G_TYPE_INSTANCE_GET_PRIVATE (                            \
    (obj),                                                 \
    KMS_TYPE_SDP_RTP_AVPF_MEDIA_HANDLER,                   \
    KmsSdpRtpAvpfMediaHandlerPrivate                       \
  )                                                        \
)

/* Object properties */
enum
{
  PROP_0,
  PROP_NACK,
  N_PROPERTIES
};

struct _KmsSdpRtpAvpfMediaHandlerPrivate
{
  gboolean nack;
};

static GObject *
kms_sdp_rtp_avpf_media_handler_constructor (GType gtype, guint n_properties,
    GObjectConstructParam * properties)
{
  GObjectConstructParam *property;
  gchar const *name;
  GObject *object;
  guint i;

  for (i = 0, property = properties; i < n_properties; ++i, ++property) {
    name = g_param_spec_get_name (property->pspec);
    if (g_strcmp0 (name, "proto") == 0) {
      if (g_value_get_string (property->value) == NULL) {
        /* change G_PARAM_CONSTRUCT_ONLY value */
        g_value_set_string (property->value, SDP_MEDIA_RTP_AVPF_PROTO);
      }
    }
  }

  object =
      G_OBJECT_CLASS (parent_class)->constructor (gtype, n_properties,
      properties);

  return object;
}

static gboolean
is_supported_encoder (const gchar * codec)
{
  guint i, len;

  len = G_N_ELEMENTS (video_rtcp_fb_enc);

  for (i = 0; i < len; i++) {
    if (g_str_has_prefix (codec, video_rtcp_fb_enc[i])) {
      return TRUE;
    }
  }

  return FALSE;
}

static gboolean
kms_sdp_rtp_avpf_media_handler_rtcp_fb_attrs (KmsSdpMediaHandler * handler,
    GstSDPMedia * media, const gchar * fmt, const gchar * enc, GError ** error)
{
  KmsSdpRtpAvpfMediaHandler *self = KMS_SDP_RTP_AVPF_MEDIA_HANDLER (handler);
  gchar *attr;

  /* Add rtcp-fb attributes */

  if (!self->priv->nack) {
    goto no_nack;
  }

  attr = g_strdup_printf ("%s %s", fmt, SDP_MEDIA_RTCP_FB_NACK);

  if (gst_sdp_media_add_attribute (media, SDP_MEDIA_RTCP_FB,
          attr) != GST_SDP_OK) {
    g_set_error (error, KMS_SDP_AGENT_ERROR, SDP_AGENT_UNEXPECTED_ERROR,
        "Can add media attribute a=%s", attr);
    g_free (attr);
    return FALSE;
  }

  g_free (attr);
  attr = g_strdup_printf ("%s %s %s", fmt /* format */ , SDP_MEDIA_RTCP_FB_NACK,
      SDP_MEDIA_RTCP_FB_PLI);

  if (gst_sdp_media_add_attribute (media, SDP_MEDIA_RTCP_FB,
          attr) != GST_SDP_OK) {
    g_set_error (error, KMS_SDP_AGENT_ERROR, SDP_AGENT_UNEXPECTED_ERROR,
        "Can add media attribute a=%s", attr);
    g_free (attr);
    return FALSE;
  }

  g_free (attr);

no_nack:
  attr =
      g_strdup_printf ("%s %s %s", fmt, SDP_MEDIA_RTCP_FB_CCM,
      SDP_MEDIA_RTCP_FB_FIR);
  if (gst_sdp_media_add_attribute (media, SDP_MEDIA_RTCP_FB,
          attr) != GST_SDP_OK) {
    g_set_error (error, KMS_SDP_AGENT_ERROR, SDP_AGENT_UNEXPECTED_ERROR,
        "Can add media attribute a=%s", attr);
    g_free (attr);
    return FALSE;
  }

  g_free (attr);

  if (g_str_has_prefix (enc, "VP8")) {
    /* Chrome adds goog-remb attribute */
    attr = g_strdup_printf ("%s %s", fmt, SDP_MEDIA_RTCP_FB_GOOG_REMB);

    if (gst_sdp_media_add_attribute (media, SDP_MEDIA_RTCP_FB,
            attr) != GST_SDP_OK) {
      g_set_error (error, KMS_SDP_AGENT_ERROR, SDP_AGENT_UNEXPECTED_ERROR,
          "Can add media attribute a=%s", attr);
      g_free (attr);
      return FALSE;
    }

    g_free (attr);
  }

  return TRUE;
}

static gboolean
kms_sdp_rtp_avpf_media_handler_add_rtcp_fb_attrs (KmsSdpMediaHandler * handler,
    GstSDPMedia * media, GError ** error)
{
  const gchar *media_str;
  guint i;

  media_str = gst_sdp_media_get_media (media);

  if (g_strcmp0 (media_str, "video") != 0) {
    /* Only nack video rtcp_fb attributes are supported */
    return TRUE;
  }

  for (i = 0;; i++) {
    const gchar *val;
    gchar **codec;

    val = gst_sdp_media_get_attribute_val_n (media, "rtpmap", i);

    if (val == NULL) {
      break;
    }

    codec = g_strsplit (val, " ", 0);
    if (!is_supported_encoder (codec[1] /* encoder */ )) {
      g_strfreev (codec);
      continue;
    }

    if (!kms_sdp_rtp_avpf_media_handler_rtcp_fb_attrs (handler, media,
            codec[0] /* format */ ,
            codec[1] /* encoder */ , error)) {
      g_strfreev (codec);
      return FALSE;
    }

    g_strfreev (codec);
  }

  return TRUE;
}

static GstSDPMedia *
kms_sdp_rtp_avpf_media_handler_create_offer (KmsSdpMediaHandler * handler,
    const gchar * media, GError ** error)
{
  GError *tmp_error = NULL;
  GstSDPMedia *m;

  m = KMS_SDP_MEDIA_HANDLER_CLASS (parent_class)->create_offer (handler, media,
      &tmp_error);

  if (tmp_error != NULL) {
    g_propagate_error (error, tmp_error);
    goto error;
  }

  if (!kms_sdp_rtp_avpf_media_handler_add_rtcp_fb_attrs (handler, m, error)) {
    goto error;
  }

  return m;

error:
  if (m != NULL) {
    gst_sdp_media_free (m);
  }

  return NULL;
}

static gboolean
format_supported (const GstSDPMedia * media, const gchar * fmt)
{
  guint i, len;

  len = gst_sdp_media_formats_len (media);

  for (i = 0; i < len; i++) {
    const gchar *format;

    format = gst_sdp_media_get_format (media, i);

    if (g_strcmp0 (format, fmt) == 0) {
      return TRUE;
    }
  }

  return FALSE;
}

static gboolean
supported_rtcp_fb_val (const gchar * val)
{
  return g_strcmp0 (val, SDP_MEDIA_RTCP_FB_GOOG_REMB) == 0 ||
      g_strcmp0 (val, SDP_MEDIA_RTCP_FB_NACK) == 0 ||
      g_strcmp0 (val, SDP_MEDIA_RTCP_FB_CCM) == 0;

}

static gboolean
kms_sdp_rtp_avpf_media_handler_filter_rtcp_fb_attrs (KmsSdpMediaHandler *
    handler, GstSDPMedia * answer, GError ** error)
{
  KmsSdpRtpAvpfMediaHandler *self = KMS_SDP_RTP_AVPF_MEDIA_HANDLER (handler);
  guint i, processed, len;

  i = processed = 0;
  len = gst_sdp_media_attributes_len (answer);

  while (processed < len) {
    const GstSDPAttribute *attr;
    gchar **opts;

    attr = gst_sdp_media_get_attribute (answer, i);

    if (attr->key == NULL) {
      return TRUE;
    }

    if (g_strcmp0 (attr->key, SDP_MEDIA_RTCP_FB) != 0) {
      i++;
      processed++;
      continue;
    }

    opts = g_strsplit (attr->value, " ", 0);
    if (!format_supported (answer, opts[0] /* format */ )) {
      /* remove rtcp-fb attribute */
      if (gst_sdp_media_remove_attribute (answer, i) != GST_SDP_OK) {
        g_set_error (error, KMS_SDP_AGENT_ERROR, SDP_AGENT_UNEXPECTED_ERROR,
            "Can not remove unsupported attribute a=rtcp-fb:%s", attr->value);
        g_strfreev (opts);
        return FALSE;
      }

      g_strfreev (opts);
      processed++;
      continue;
    }

    if (g_strcmp0 (opts[1] /* rtcp-fb-val */ , SDP_MEDIA_RTCP_FB_NACK) == 0
        && !self->priv->nack) {
      /* remove rtcp-fb nack attribute */
      if (gst_sdp_media_remove_attribute (answer, i) != GST_SDP_OK) {
        g_set_error (error, KMS_SDP_AGENT_ERROR, SDP_AGENT_UNEXPECTED_ERROR,
            "Can not remove unsupported attribute a=rtcp-fb:%s", attr->value);
        g_strfreev (opts);
        return FALSE;
      }

      g_strfreev (opts);
      processed++;
      continue;
    }

    if (!supported_rtcp_fb_val (opts[1] /* rtcp-fb-val */ )) {
      GST_DEBUG ("%d Removing unsupported attribute a=rtcp-fb:%s", i,
          attr->value);

      if (gst_sdp_media_remove_attribute (answer, i) != GST_SDP_OK) {
        g_set_error (error, KMS_SDP_AGENT_ERROR, SDP_AGENT_UNEXPECTED_ERROR,
            "Can not remove unsupported attribute a=rtcp-fb:%s", attr->value);
        g_strfreev (opts);
        return FALSE;
      }

      g_strfreev (opts);
      processed++;
      continue;
    }

    g_strfreev (opts);
    i++;
    processed++;
  }

  return TRUE;
}

GstSDPMedia *
kms_sdp_rtp_avpf_media_handler_create_answer (KmsSdpMediaHandler * handler,
    const GstSDPMedia * offer, GError ** error)
{
  GError *tmp_error = NULL;
  GstSDPMedia *m;

  m = KMS_SDP_MEDIA_HANDLER_CLASS (parent_class)->create_answer (handler, offer,
      &tmp_error);

  if (tmp_error != NULL) {
    g_propagate_error (error, tmp_error);
    goto error;
  }

  /* By default, RTP/AVP adds all attributes that it is not capable */
  /* of manage. We should filter those related to RTP/AVPF protocol */
  if (!kms_sdp_rtp_avpf_media_handler_filter_rtcp_fb_attrs (handler, m, error)) {
    goto error;
  }

  return m;

error:
  if (m != NULL) {
    gst_sdp_media_free (m);
  }

  return NULL;
}

static void
kms_sdp_rtp_avpf_media_handler_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  KmsSdpRtpAvpfMediaHandler *self = KMS_SDP_RTP_AVPF_MEDIA_HANDLER (object);

  switch (prop_id) {
    case PROP_NACK:
      g_value_set_boolean (value, self->priv->nack);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
kms_sdp_rtp_avpf_media_handler_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  KmsSdpRtpAvpfMediaHandler *self = KMS_SDP_RTP_AVPF_MEDIA_HANDLER (object);

  switch (prop_id) {
    case PROP_NACK:
      self->priv->nack = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
kms_sdp_rtp_avpf_media_handler_class_init (KmsSdpRtpAvpfMediaHandlerClass *
    klass)
{
  GObjectClass *gobject_class;
  KmsSdpMediaHandlerClass *handler_class;

  gobject_class = G_OBJECT_CLASS (klass);
  handler_class = KMS_SDP_MEDIA_HANDLER_CLASS (klass);

  gobject_class->constructor = kms_sdp_rtp_avpf_media_handler_constructor;
  gobject_class->get_property = kms_sdp_rtp_avpf_media_handler_get_property;
  gobject_class->set_property = kms_sdp_rtp_avpf_media_handler_set_property;

  handler_class->create_offer = kms_sdp_rtp_avpf_media_handler_create_offer;
  handler_class->create_answer = kms_sdp_rtp_avpf_media_handler_create_answer;

  g_object_class_install_property (gobject_class, PROP_NACK,
      g_param_spec_boolean ("nack", "Nack",
          "Wheter rtcp-fb-nack-param if supproted or not",
          SDP_MEDIA_RTP_AVPF_DEFAULT_NACK,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (KmsSdpRtpAvpfMediaHandlerPrivate));
}

static void
kms_sdp_rtp_avpf_media_handler_init (KmsSdpRtpAvpfMediaHandler * self)
{
  self->priv = KMS_SDP_RTP_AVPF_MEDIA_HANDLER_GET_PRIVATE (self);
}

KmsSdpRtpAvpfMediaHandler *
kms_sdp_rtp_avpf_media_handler_new (void)
{
  KmsSdpRtpAvpfMediaHandler *handler;

  handler =
      KMS_SDP_RTP_AVPF_MEDIA_HANDLER (g_object_new
      (KMS_TYPE_SDP_RTP_AVPF_MEDIA_HANDLER, NULL));

  return handler;
}
