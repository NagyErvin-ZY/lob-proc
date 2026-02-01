{{- define "feeder.fullname" -}}
{{- .Release.Name | trunc 63 | trimSuffix "-" -}}
{{- end -}}

{{- define "feeder.labels" -}}
app.kubernetes.io/name: feeder
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end -}}

{{- define "feeder.selectorLabels" -}}
app.kubernetes.io/name: feeder
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end -}}
